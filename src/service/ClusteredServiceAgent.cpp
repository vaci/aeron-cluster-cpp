#include "ClusteredServiceAgent.h"

#include "client/ClusterException.h"
#include "ClusterCounters.h"
#include "RecoveryState.h"
#include "ServiceSnapshotLoader.h"
#include "ServiceSnapshotTaker.h"
#include "client/AeronArchive.h"
#include "client/RecordingPos.h"
#include "ChannelUri.h"

namespace aeron { namespace cluster { namespace service {

using client::ClusterException;

typedef std::function<void(
    CountersReader &countersReader,
    std::int64_t registrationId,
    std::int32_t counterId)> on_unavailable_counter_t;

auto unavailableCounterHandler(ClusteredServiceAgent &agent)
{
  return [&](CountersReader &countersReader, std::int64_t registrationId, std::int32_t counterId)
  {
    return agent.onUnavailableCounter(countersReader, registrationId, counterId);
  };
}


template<typename IdleStrategy = aeron::concurrent::BackoffIdleStrategy>
std::shared_ptr<Counter> awaitCounter(CountersReader &counters, std::int32_t typeId, std::int32_t clusterId)
{
  IdleStrategy idle;

  auto counter = ClusterCounters::find(counters, typeId, clusterId);
  while (!counter)
  {
    idle.idle();
    counter = ClusterCounters::find(counters, typeId, clusterId);
    
  }
  return counter;
}


template<typename IdleStrategy = aeron::concurrent::BackoffIdleStrategy>
std::int32_t awaitRecordingCounter(
  std::int64_t sessionId,
  CountersReader &counters,
  std::shared_ptr<AeronArchive> archive)
{
  IdleStrategy idle;
  
  std::int32_t counterId = archive::client::RecordingPos::findCounterIdBySessionId(counters, sessionId);
  while (CountersReader::NULL_COUNTER_ID == counterId)
  {
    idle.idle();
    archive->checkForErrorResponse();
    counterId = archive::client::RecordingPos::findCounterIdBySessionId(counters, sessionId);
  }

  return counterId;
}


template<typename IdleStrategy = aeron::concurrent::BackoffIdleStrategy>
void awaitRecordingComplete(
  std::int64_t recordingId,
  std::int64_t position,
  CountersReader &counters,
  std::int32_t counterId,
  std::shared_ptr<AeronArchive> archive)
{
  IdleStrategy idle;
  while (counters.getCounterValue(counterId) < position)
  {
    idle.idle();
    archive->checkForErrorResponse();

    if (!archive::client::RecordingPos::isActive(counters, counterId, recordingId))
    {
      throw ClusterException(std::string("recording stopped unexpectedly: ") + std::to_string(recordingId), SOURCEINFO);
    }
  }
}

using ClusterException = client::ClusterException;

ClusteredServiceAgent::AsyncConnect::AsyncConnect(
  Context & ctx,
  std::int64_t publicationId,
  std::int64_t subscriptionId) :
  m_ctx(ctx),
  m_aeron(ctx.aeron()),
  m_aeronArchiveConnect(AeronArchive::asyncConnect(m_ctx.archiveContext())),
  m_publicationId(publicationId),
  m_subscriptionId(subscriptionId)
{
}

std::shared_ptr<ClusteredServiceAgent> ClusteredServiceAgent::AsyncConnect::poll()
{
  auto& counters = m_aeron->countersReader();
  m_agent = std::make_shared<ClusteredServiceAgent>(m_ctx);

  if (!m_publication)
  {
    m_publication = m_aeron->findExclusivePublication(m_publicationId);
  }
  if (!m_subscription)
  {
    m_subscription = m_aeron->findSubscription(m_subscriptionId);
  }
  if (m_publication && !m_agent->m_proxy)
  {
    m_agent->m_proxy = std::make_shared<ConsensusModuleProxy>(m_publication);
  }
  if (m_subscription && !m_agent->m_serviceAdapter)
  {
    m_agent->m_serviceAdapter = std::make_shared<ServiceAdapter>(m_subscription);
  }
  if (!m_agent->m_archive)
  {
    m_agent->m_archive = m_aeronArchiveConnect->poll();
  }
  if (!m_agent->m_commitPosition)
  {
    m_agent->m_commitPosition = ClusterCounters::find(
      counters,
      Configuration::COMMIT_POSITION_TYPE_ID,
      m_ctx.clusterId());
  }
  if (!m_recoveryCounter)
  {
    m_recoveryCounter = RecoveryState::findCounter(counters, m_ctx.clusterId());
  }

  if (0 == m_step &&
      m_agent->m_proxy &&
      m_agent->m_serviceAdapter &&
      m_agent->m_commitPosition &&
      m_recoveryCounter &&
      m_agent->m_archive)
  {
    if (!m_agent->m_proxy->publication()->isConnected())
    {
      return {};
    }

    // bind the service adapter to the agent
    m_agent->m_serviceAdapter->agent(m_agent);
    m_step = 1;
  }

  if (m_step == 1)
  {
    std::int32_t counterId = m_recoveryCounter->id();
    m_agent->m_logPosition = RecoveryState::getLogPosition(counters, counterId);
    m_clusterTime = RecoveryState::getTimestamp(counters, counterId);
    m_leadershipTermId = RecoveryState::getLeadershipTermId(counters, counterId);
    m_snapshotRecordingId = RecoveryState::getSnapshotRecordingId(counters, counterId, m_ctx.serviceId());

    if (m_leadershipTermId != NULL_VALUE)
    {
      // start snapshot replay
      auto& channel = m_ctx.replayChannel();
      std::int32_t streamId = m_ctx.replayStreamId();
      m_snapshotSessionId = (int)m_agent->m_archive->startReplay(m_snapshotRecordingId, 0, NULL_VALUE, channel, streamId);
      auto replaySessionChannel = ChannelUri::addSessionId(channel, m_snapshotSessionId);
      m_snapshotSubscriptionId = m_aeron->addSubscription(replaySessionChannel, streamId);
      m_step = 2;
    }
    else {
      // skip snapshot replay
      m_step = 3;
    }
    return {};
  }

  if (m_step == 2)
  {
    if (!m_snapshotSubscription)
    {
      m_snapshotSubscription = m_aeron->findSubscription(m_snapshotSubscriptionId);
      return {};
    }

    if (!m_snapshotImage)
    {
      m_snapshotImage = m_snapshotSubscription->imageBySessionId(m_snapshotSessionId);
      return {};
    }

    if (!m_snapshotLoader)
    {
      m_snapshotLoader = std::make_unique<ServiceSnapshotLoader>(m_snapshotImage, *m_agent);
      return {};
    }

    if (!m_snapshotLoader->isDone())
    {
      std::int32_t fragments = m_snapshotLoader->poll();
      if (fragments == 0)
      {
	m_agent->m_archive->checkForErrorResponse();
	if (m_snapshotImage->isClosed())
	{
	  throw ClusterException("snapshot ended unexpectedly: ", SOURCEINFO);
	}
      }
      return {};
    }

    m_step = 3;
    return {};
  }

  if (m_step == 3)
  {
    // m_snapshotImage may be null if no snapshot
    if (m_agent->m_service->onStart(*m_agent, m_snapshotImage))
    {
      m_step = 4;
    }
    return {};
  }

  if (m_step == 4)
  {
    m_agent->ack( m_aeron->clientId());
    
    m_agent->m_isServiceActive = true;
    return m_agent;
  }

  return {};
}

void ClusteredServiceAgent::onUnavailableCounter(
  CountersReader &countersReader,
  std::int64_t registrationId,
  std::int32_t counterId)
{
  if (m_commitPosition != nullptr && m_commitPosition->registrationId() == registrationId && m_commitPosition->id() == counterId)
  {
    m_commitPosition = nullptr;
  }
}

void ClusteredServiceAgent::addSession(std::unique_ptr<ContainerClientSession> session)
{
  std::int64_t clusterSessionId = session->id();
  m_sessionByIdMap.insert({clusterSessionId, session.get()});

  auto index = std::lower_bound(
    m_sessions.begin(), m_sessions.end(), clusterSessionId,
    [](auto& session, std::int64_t id)
    {
      return session->id() < id;
    }
  );
  
  m_sessions.insert(index, std::move(session));
}

std::shared_ptr<ClusteredServiceAgent::AsyncConnect> ClusteredServiceAgent::asyncConnect(Context &ctx)
{
  ctx.conclude();

  std::shared_ptr<Aeron> aeron = ctx.aeron();
  
  const std::int64_t publicationId = aeron->addExclusivePublication(
    ctx.controlChannel(), ctx.consensusModuleStreamId());

  const std::int64_t subscriptionId = aeron->addSubscription(
    ctx.controlChannel(), ctx.serviceStreamId());
  
  return std::make_shared<AsyncConnect>(ctx, publicationId, subscriptionId);

}

ClusteredServiceAgent::ClusteredServiceAgent(
  Context &ctx) :
  m_ctx(ctx),
  m_aeron(ctx.aeron()),
  m_nanoClock(systemNanoClock),
  m_epochClock(currentTimeMillis),
  m_service(ctx.clusteredService()),
  m_serviceId(ctx.serviceId()),
  m_subscriptionAlias(std::string("log-sc-") + std::to_string(ctx.serviceId())),
  m_standbySnapshotFlags(ctx.standbySnapshotEnabled()
    ? Configuration::CLUSTER_ACTION_FLAGS_STANDBY_SNAPSHOT
    : Configuration::CLUSTER_ACTION_FLAGS_DEFAULT),
  m_logAdapter(std::make_unique<BoundedLogAdapter>(*this, m_ctx.logFragmentLimit()))
{
}

std::shared_ptr<ClientSession> ClusteredServiceAgent::getClientSession(std::int64_t clusterSessionId)
{
  return nullptr;
}

void ClusteredServiceAgent::doWork()
{
  if (m_currentSnapshot)
  {
    if (!m_currentSnapshot->doWork())
    {
      return;
    }
    m_currentSnapshot = nullptr;
  }
    
  processAckQueue();
}

void ClusteredServiceAgent::ackDone(std::int64_t relevantId)
{
  if (relevantId == NULL_VALUE)
  {
    m_requestedAckPosition = archive::client::NULL_POSITION;
  }
}

void ClusteredServiceAgent::ack(std::int64_t relevantId)
{
  std::int64_t id = m_ackId++;
  std::int64_t timestamp = m_clusterTime;
    
  if (m_ackQueue.empty() && m_proxy->ack(
      m_logPosition,
      m_clusterTime,
      id,
      relevantId,
      m_serviceId))
  {
    ackDone(relevantId);
    return;
  }
  else
  {
    m_ackQueue.push({m_logPosition, m_clusterTime, id, relevantId, m_serviceId});
  }
  
}

void ClusteredServiceAgent::processAckQueue()
{
  while (!m_ackQueue.empty())
  {
    auto& ack = m_ackQueue.front();
    if (m_proxy->ack(
	ack.m_logPosition,
	ack.m_timestamp,
	ack.m_ackId,
	ack.m_relevantId,
	ack.m_serviceId))
    {
      ackDone(ack.m_relevantId);
      m_ackQueue.pop();      
    }
    else
    {
      break;
    }
  }
}

void ClusteredServiceAgent::onNewLeadershipTermEvent(
  std::int64_t leadershipTermId,
  std::int64_t logPosition,
  std::int64_t timestamp,
  std::int64_t termBaseLogPosition,
  std::int32_t leaderMemberId,
  std::int32_t logSessionId,
  std::int32_t appVersion)
{
  /* TODO
  if (!m_ctx.appVersionValidator().isVersionCompatible(ctx.appVersion(), appVersion))
  {
    m_ctx.countedErrorHandler().onError(new ClusterException(
	"incompatible version: " + SemanticVersion.toString(ctx.appVersion()) +
	" log=" + SemanticVersion.toString(appVersion)));
    throw new AgentTerminationException();
  }
  */

  m_leadershipTermId = leadershipTermId;
  m_logPosition = logPosition;
  m_clusterTime = timestamp;

  m_service->onNewLeadershipTermEvent(
    leadershipTermId,
    logPosition,
    timestamp,
    termBaseLogPosition,
    leaderMemberId,
    logSessionId,
    // TODO timeUnit
    appVersion);
}

bool ClusteredServiceAgent::closeClientSession(std::int64_t clusterSessionId)
{
  checkForValidInvocation();

  auto clientSession = m_sessionByIdMap.find(clusterSessionId);
  if (clientSession == m_sessionByIdMap.end())
  {
    throw ClusterException(
      std::string("unknown clusterSessionId: ") + std::to_string(clusterSessionId), SOURCEINFO);
  }

  if (clientSession->second->isClosing())
  {
    return true;
  }

  if (m_proxy->closeSession(clusterSessionId))
  {
    clientSession->second->markClosing();
    return true;
  }
  
  return false;
}

std::int64_t ClusteredServiceAgent::offer(AtomicBuffer& message)
{
  checkForValidInvocation();
  std::array<std::uint8_t, SessionMessageHeader::sbeBlockAndHeaderLength()> buffer;

  SessionMessageHeader()
    .wrapAndApplyHeader(reinterpret_cast<char*>(buffer.begin()), 0, buffer.size())
    .clusterSessionId(context().serviceId());

  AtomicBuffer atomicBuffer;
  return m_proxy->offer(atomicBuffer, message);
}


void ClusteredServiceAgent::onTimerEvent(
  std::int64_t logPosition, std::int64_t correlationId, std::int64_t timestamp)
{
  m_logPosition = logPosition;
  m_clusterTime = timestamp;
  m_service->onTimerEvent(correlationId, timestamp);
}

void ClusteredServiceAgent::role(Cluster::Role newRole)
{
  if (newRole != m_role)
  {
    m_role = newRole;
    m_activeLifecycleCallbackName = "onRoleChange";
    try
    {
      m_service->onRoleChange(newRole);
    }
    catch (...)
    {
      m_activeLifecycleCallbackName = nullptr;
      throw;
    }
    m_activeLifecycleCallbackName = nullptr;
  }
}


void ClusteredServiceAgent::onTakeSnapshot(std::int64_t logPosition, std::int64_t leadershipTermId)
{
  m_currentSnapshot = std::make_unique<SnapshotState>(*this);
  m_currentSnapshot->m_publicationId = m_aeron->addExclusivePublication(
      m_ctx.snapshotChannel(), m_ctx.snapshotStreamId());
}

void ClusteredServiceAgent::disconnectEgress()
{
  for (auto && entry: m_sessionByIdMap)
  {
    entry.second->disconnect();
  }
}

void ClusteredServiceAgent::checkForValidInvocation()
{
  if (nullptr != m_activeLifecycleCallbackName)
  {
    throw ClusterException(
      std::string("sending messages or scheduling timers is not allowed from ") + m_activeLifecycleCallbackName, SOURCEINFO);
  }

  if (m_isBackgroundInvocation)
  {
    throw ClusterException(
      std::string("sending messages or scheduling timers is not allowed from ClusteredService.doBackgroundWork"), SOURCEINFO);
  }
}

void ClusteredServiceAgent::snapshotState(
  std::shared_ptr<ExclusivePublication> publication,
  std::int64_t logPosition,
  std::int64_t leadershipTermId)
{
  ServiceSnapshotTaker snapshotTaker(publication, m_aeron);

  snapshotTaker.markBegin(Configuration::SNAPSHOT_TYPE_ID, logPosition, leadershipTermId, 0, m_ctx.appVersion());

  for (auto&& session: m_sessions)
  {
    snapshotTaker.snapshotSession(*session);
  }

  snapshotTaker.markEnd(Configuration::SNAPSHOT_TYPE_ID, logPosition, leadershipTermId, 0, m_ctx.appVersion());
}

bool ClusteredServiceAgent::checkForClockTick(std::int64_t nowNs)
{
  if (m_isAbort || m_aeron->isClosed())
  {
    m_isAbort = true;
    throw ClusterException("unexpected Aeron close", SOURCEINFO);
  }

  if (nowNs - m_lastSlowTickNs > (1000*1000)) // 1ms
  {
    m_lastSlowTickNs = nowNs;
    
    auto& invoker = m_aeron->conductorAgentInvoker();
    invoker.invoke();
    if (m_isAbort || m_aeron->isClosed())
    {
      m_isAbort = true;
      throw ClusterException("unexpected Aeron close", SOURCEINFO);
    }

    auto& counters = m_aeron->countersReader();
    if (m_commitPosition != nullptr && m_commitPosition->isClosed())
    {
      // TODO
      //m_ctx.errorLog().record(new AeronEvent(
      //"commit-pos counter unexpectedly closed, terminating", AeronException.Category.WARN));
      throw ClusterException("Closed", SOURCEINFO);
    }

    std::int64_t nowMs = m_epochClock();
    if (nowMs >= m_markFileUpdateDeadlineMs)
    {
      m_markFileUpdateDeadlineMs = nowMs + 1; // TODO MARK_FILE_UPDATE_INTERVAL_MS;
      // TODO
      //m_markFile.updateActivityTimestamp(nowMs);
    }

    return true;
  }

  return false;
}

bool ClusteredServiceAgent::SnapshotState::doWork()
{
  auto& counters = m_agent.m_aeron->countersReader();

  if (!m_publication)
  {
    m_publication = m_agent.m_aeron->findExclusivePublication(m_publicationId);
    if (!m_publication)
    {
      return false;
    }
    auto channel = ChannelUri::addSessionId(
      m_agent.context().snapshotChannel(), m_publication->sessionId());
    m_agent.m_archive->startRecording(channel, m_agent.m_ctx.snapshotStreamId(), AeronArchive::LOCAL, true);
  }
  
  if (m_recordingCounterId == CountersReader::NULL_COUNTER_ID)
  {
    auto sessionId = m_publication->sessionId();
    m_recordingCounterId = archive::client::RecordingPos::findCounterIdBySessionId(counters, sessionId);
    if (m_recordingCounterId == CountersReader::NULL_COUNTER_ID)
    {
      m_agent.m_archive->checkForErrorResponse();
      return false;
    }
    m_recordingId = archive::client::RecordingPos::getRecordingId(counters, m_recordingCounterId);
  }

  if (!m_snapshotComplete)
  {
    m_snapshotComplete = m_agent.m_service->onTakeSnapshot(m_publication);
    if (!m_snapshotComplete)
      return false;;
  }

  if (counters.getCounterValue(m_recordingCounterId) < m_publication->position())
  {
    return false;
  }

  m_agent.ack(m_recordingId);
  return true;
}

int ClusteredServiceAgent::pollServiceAdapter()
{
  int workCount = 0;

  workCount += m_serviceAdapter->poll();
  
  if (nullptr != m_activeLogEvent && nullptr == m_logAdapter->image())
  {
    auto event = std::move(m_activeLogEvent);
    m_activeLogEvent = nullptr;
    // TODO
    // joinActiveLog(event);
  }

  if (NULL_POSITION != m_terminationPosition && m_logPosition >= m_terminationPosition)
  {
    if (m_logPosition > m_terminationPosition)
    {
      //TODO
      // ctx.countedErrorHandler().onError(new ClusterEvent(
      //  "service terminate: logPosition=" + logPosition + " > terminationPosition=" + terminationPosition));
    }

    terminate(m_logPosition == m_terminationPosition);
  }

  if (NULL_POSITION != m_requestedAckPosition && m_logPosition >= m_requestedAckPosition)
  {
    if (m_logPosition > m_requestedAckPosition)
    {
      // TODO
      /*
      ctx.countedErrorHandler().onError(new ClusterEvent(
	  "service terminate: logPosition=" + logPosition +
	  " > requestedAckPosition=" + terminationPosition));
      */
    }

    ack(NULL_VALUE);
  }

  return workCount;
}

void ClusteredServiceAgent::onSessionOpen(
  std::int64_t leadershipTermId,
  std::int64_t logPosition,
  std::int64_t clusterSessionId,
  std::int64_t timestamp,
  std::int32_t responseStreamId,
  const std::string &responseChannel,
  const std::vector<char> &encodedPrincipal)
{
  // TODO
}

void ClusteredServiceAgent::terminate(bool expected)
{
  // TODO
}


}}}
