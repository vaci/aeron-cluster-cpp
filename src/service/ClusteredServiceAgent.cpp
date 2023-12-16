#include "ClusteredServiceAgent.h"
#include "ChannelUriStringBuilder.h"
#include "client/ClusterException.h"
#include "ClusterCounters.h"
#include "RecoveryState.h"
#include "ServiceSnapshotLoader.h"
#include "ServiceSnapshotTaker.h"
#include "client/AeronArchive.h"
#include "client/RecordingPos.h"
#include "ChannelUri.h"

namespace aeron { namespace cluster { namespace service {

using namespace codecs;

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
  Context & ctx) :
  m_ctx(ctx),
  m_aeronArchiveConnect(AeronArchive::asyncConnect(m_ctx.archiveContext()))
{
  m_publicationId = m_ctx.aeron()->addExclusivePublication(
    m_ctx.controlChannel(), m_ctx.consensusModuleStreamId());

  m_subscriptionId = m_ctx.aeron()->addSubscription(
    m_ctx.controlChannel(), m_ctx.serviceStreamId());

  m_agent = std::make_shared<ClusteredServiceAgent>(m_ctx);
}

std::shared_ptr<ClusteredServiceAgent> ClusteredServiceAgent::AsyncConnect::poll()
{
  auto& counters = m_ctx.aeron()->countersReader();

  if (!m_publication)
  {
    m_publication = m_ctx.aeron()->findExclusivePublication(m_publicationId);
  }
  if (!m_subscription)
  {
    m_subscription = m_ctx.aeron()->findSubscription(m_subscriptionId);
  }
  if (m_publication && !m_agent->m_proxy)
  {
    std::cout << "Publishing to proxy on : " << m_publication->channel() << ":" << m_publication->streamId() << std::endl;
    m_agent->m_proxy = std::make_shared<ConsensusModuleProxy>(m_publication);
  }
  if (m_subscription && !m_agent->m_serviceAdapter)
  {
    std::cout << "Subscribed to service: " << m_subscription->channel() << ":" << m_subscription->streamId() << std::endl;
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
  if (m_recoveryCounterId == CountersReader::NULL_COUNTER_ID)
  {
    m_recoveryCounterId = RecoveryState::findCounter(counters, m_ctx.clusterId());
  }

  if (0 == m_step &&
      m_agent->m_proxy &&
      m_agent->m_serviceAdapter &&
      m_agent->m_commitPosition &&
      m_recoveryCounterId != CountersReader::NULL_COUNTER_ID &&
      m_agent->m_archive)
  {
    std::cout << "Resolved all parts" << std::endl;
    if (!m_agent->m_proxy->publication()->isConnected())
    {
      std::cout << "Not connected:" << m_agent->m_proxy->publication()->channel() << " : " << m_agent->m_proxy->publication()->streamId() << std::endl;
      return {};
    }

    // bind the service adapter to the agent
    m_agent->m_serviceAdapter->agent(m_agent);
    m_step = 1;
  }

  if (m_step == 1)
  {
    std::cout << "step 1" << std::endl;
    m_agent->m_logPosition = RecoveryState::getLogPosition(counters, m_recoveryCounterId);
    m_agent->m_clusterTime = RecoveryState::getTimestamp(counters, m_recoveryCounterId);
    m_agent->m_leadershipTermId = RecoveryState::getLeadershipTermId(counters, m_recoveryCounterId);

    if (m_agent->m_leadershipTermId != NULL_VALUE)
    {
      m_snapshotRecordingId = RecoveryState::getSnapshotRecordingId(
	counters, m_recoveryCounterId, m_ctx.serviceId());

      std::cout << "Starting snapshot replay: recordingId=" << m_snapshotRecordingId << std::endl;
      // start snapshot replay
      auto& channel = m_ctx.replayChannel();
      std::int32_t streamId = m_ctx.replayStreamId();
      m_snapshotSessionId = (int)m_agent->m_archive->startReplay(m_snapshotRecordingId, 0, NULL_VALUE, channel, streamId);
      auto replaySessionChannel = ChannelUri::addSessionId(channel, m_snapshotSessionId);
      m_snapshotSubscriptionId = m_ctx.aeron()->addSubscription(replaySessionChannel, streamId);
      m_step = 2;
    }
    else {
      std::cout << "No snapshot to replay" << std::endl;
      // skip snapshot replay
      m_step = 3;
    }
    return {};
  }

  if (m_step == 2)
  {
    std::cout << "step 2" << std::endl;
    if (!m_snapshotSubscription)
    {
      m_snapshotSubscription = m_ctx.aeron()->findSubscription(m_snapshotSubscriptionId);
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
    
    std::cout << "step 3" << std::endl;
    // m_snapshotImage may be null if no snapshot
    if (m_agent->m_service->onStart(*m_agent, m_snapshotImage))
    {
      m_step = 4;
    }
    return {};
  }

  if (m_step == 4)
  {
    std::cout << "step 4" << std::endl;
    m_agent->ack(m_ctx.aeron()->clientId());
    
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
  return std::make_shared<AsyncConnect>(ctx);
}

ClusteredServiceAgent::ClusteredServiceAgent(
  Context &ctx) :
  m_ctx(ctx),
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
  if (m_service == nullptr)
  {
    throw ClusterException("Service is null", SOURCEINFO);
  }
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

  pollServiceAdapter();

  if (m_logAdapter->image() != nullptr)
  {
    auto position = m_commitPosition->get();
    std::cout << "Polling log adapter: " << position << std::endl;
    m_logAdapter->poll(position);
  }
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

  std::cout << "Ack: " << id << std::endl;
  if (m_ackQueue.empty() && m_proxy->ack(
      m_logPosition,
      m_clusterTime,
      id,
      relevantId,
      m_serviceId))
  {

    std::cout << "Ack done: " << id << std::endl;
    ackDone(relevantId);
    return;
  }
  else
  {
    std::cout << "Ack queued: " << relevantId << std::endl;
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
  // TODO
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
  m_currentSnapshot->m_publicationId = m_ctx.aeron()->addExclusivePublication(
      m_ctx.snapshotChannel(), m_ctx.snapshotStreamId());
}

void ClusteredServiceAgent::disconnectEgress(exception_handler_t errorHandler)
{
  for (auto && entry: m_sessionByIdMap)
  {
    entry.second->disconnect(errorHandler);
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
  ServiceSnapshotTaker snapshotTaker(publication);

  snapshotTaker.markBegin(Configuration::SNAPSHOT_TYPE_ID, logPosition, leadershipTermId, 0, m_ctx.appVersion());

  for (auto&& session: m_sessions)
  {
    snapshotTaker.snapshotSession(*session);
  }

  snapshotTaker.markEnd(Configuration::SNAPSHOT_TYPE_ID, logPosition, leadershipTermId, 0, m_ctx.appVersion());
}

bool ClusteredServiceAgent::checkForClockTick(std::int64_t nowNs)
{
  if (m_isAbort || m_ctx.aeron()->isClosed())
  {
    m_isAbort = true;
    throw ClusterException("unexpected Aeron close", SOURCEINFO);
  }

  if (nowNs - m_lastSlowTickNs > (1000*1000)) // 1ms
  {
    m_lastSlowTickNs = nowNs;
    
    auto& invoker = m_ctx.aeron()->conductorAgentInvoker();
    invoker.invoke();
    if (m_isAbort || m_ctx.aeron()->isClosed())
    {
      m_isAbort = true;
      throw ClusterException("unexpected Aeron close", SOURCEINFO);
    }

    auto& counters = m_ctx.aeron()->countersReader();
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

ClusteredServiceAgent::SnapshotState::SnapshotState(ClusteredServiceAgent& agent) :
  m_agent(agent)
{
}

bool ClusteredServiceAgent::SnapshotState::doWork()
{
  auto& counters = m_agent.context().aeron()->countersReader();

  if (!m_publication)
  {
    m_publication = m_agent.context().aeron()->findExclusivePublication(m_publicationId);
    if (!m_publication)
    {
      return false;
    }
    auto channel = ChannelUri::addSessionId(
      m_agent.context().snapshotChannel(), m_publication->sessionId());

    std::cout << "Starting snapshot recording" << channel << std::endl;
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

  std::cout << "Polling service adapter" << std::endl;
  workCount += m_serviceAdapter->poll();
  
  if (nullptr != m_activeLogEvent && nullptr == m_logAdapter->image())
  {
    std::cout << "Joining active log event" << std::endl;
    if (joinActiveLog(*m_activeLogEvent))
    {
      m_activeLogEvent = nullptr;
      return workCount;
    }
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
  std::cout << "ClusteredServiceAgent::onSessionOpen" << std::endl;
  m_logPosition = logPosition;
  m_clusterTime = timestamp;

  if (m_sessionByIdMap.find(clusterSessionId) != m_sessionByIdMap.end())
  {
    throw ClusterException(std::string("clashing open clusterSessionId=") + std::to_string(clusterSessionId) +
      " leadershipTermId=" + std::to_string(leadershipTermId) + " logPosition=" + std::to_string(logPosition), SOURCEINFO);
  }

  auto session = std::make_unique<ContainerClientSession>(
    clusterSessionId, responseStreamId, responseChannel, encodedPrincipal, *this);

  if (Cluster::Role::LEADER == m_role && m_ctx.isRespondingService())
  {
    session->connect(context().aeron());
  }

  auto session_p = session.get();
  addSession(std::move(session));
  m_service->onSessionOpen(*session_p, timestamp);
}

std::unique_ptr<ContainerClientSession> ClusteredServiceAgent::removeSession(std::int64_t clusterSessionId)
{
  m_sessionByIdMap.erase(clusterSessionId);

  auto iter = std::find_if(m_sessions.begin(), m_sessions.end(),
    [clusterSessionId](auto& value) { return value->id() == clusterSessionId; });

  if (iter != m_sessions.end())
  {
    auto session = std::move(*iter);
    m_sessions.erase(iter);
    return session;
  }
  else
  {
    return nullptr;
  }
}

void ClusteredServiceAgent::onSessionClose(
  std::int64_t leadershipTermId,
  std::int64_t logPosition,
  std::int64_t clusterSessionId,
  std::int64_t timestamp,
  CloseReason closeReason)
{
  std::cout << "ClusteredServiceAgent::onSessionClose" << std::endl;
  m_logPosition = logPosition;
  m_clusterTime = timestamp;

  auto session = removeSession(clusterSessionId);
  if (session == nullptr)
  {
    throw ClusterException(
      std::string("unknown clusterSessionId=") + std::to_string(clusterSessionId) + " for close reason=" + std::to_string(closeReason) +
      " leadershipTermId=" + std::to_string(leadershipTermId) + " logPosition=" + std::to_string(logPosition), SOURCEINFO);
  }

  session->disconnect(m_ctx.errorHandler());
  m_service->onSessionClose(*session, timestamp, closeReason);
}

void ClusteredServiceAgent::onServiceTerminationPosition(std::int64_t logPosition)
{
  std::cout << "ClusteredServiceAgent::onServiceTerminationPosition" << std::endl;
  m_terminationPosition = logPosition;
}

void ClusteredServiceAgent::onServiceAction(
  std::int64_t leadershipTermId,
  std::int64_t logPosition,
  std::int64_t timestamp,
  ClusterAction::Value action,
  std::int32_t  flags)
{
  std::cout << "ClusteredServiceAgent::onServiceAction" << std::endl;
  m_logPosition = logPosition;
  m_clusterTime = timestamp;
  executeAction(action, logPosition, leadershipTermId, flags);
}

void ClusteredServiceAgent::onMembershipChange(
  std::int64_t logPosition,
  std::int64_t timestamp,
  codecs::ChangeType::Value changeType,
  std::int32_t memberId)
{
  std::cout << "ClusteredServiceAgent::onMembershipChange" << std::endl;
  m_logPosition = logPosition;
  m_clusterTime = timestamp;
    
  if (memberId == m_memberId && changeType == ChangeType::Value::QUIT)
  {
    terminate(true);
  }
}
  
void ClusteredServiceAgent::onJoinLog(
  std::int64_t logPosition,
  std::int64_t maxLogPosition,
  std::int32_t memberId,
  std::int32_t logSessionId,
  std::int32_t logStreamId,
  bool isStartup,
  Cluster::Role role,
  const std::string &logChannel)
{
  std::cout << "ClusteredServiceAgent::onJoinLog" << std::endl;

  m_activeLogEvent = std::make_unique<ActiveLogEvent>(
    logPosition,
    maxLogPosition,
    memberId,
    logSessionId,
    logStreamId,
    isStartup,
    role,
    logChannel);
}

void ClusteredServiceAgent::terminate(bool expected)
{
  // TODO
}

bool ClusteredServiceAgent::joinActiveLog(ActiveLogEvent &activeLog)
{
  if (Cluster::Role::LEADER != activeLog.m_role)
  {
    // TODO
    //disconnectEgress(ctx.countedErrorHandler());
  }

  if (activeLog.m_logSubscriptionId == NULL_VALUE)
  {
  
    std::cout << "Joining active log event, adding log subscription" << std::endl;
    auto channel = ChannelUri::parse(activeLog.m_channel);
    channel->put(ALIAS_PARAM_NAME, m_subscriptionAlias);
  
    activeLog.m_logSubscriptionId = context().aeron()->addSubscription(channel->toString(), activeLog.m_streamId);
    return false;
  }

  if (activeLog.m_subscription == nullptr)
  {
    std::cout << "Joining active log event, finding log subscription" << std::endl;
    activeLog.m_subscription = context().aeron()->findSubscription(activeLog.m_logSubscriptionId);
    return false;
  }

  if (activeLog.m_image == nullptr)
  {
    std::cout << "Joining active log event, finding log image" << std::endl;
    activeLog.m_image = activeLog.m_subscription->imageBySessionId(activeLog.m_sessionId);
    return false;
  }

  if (activeLog.m_image->joinPosition() != m_logPosition)
    {
      throw ClusterException(
	std::string("Cluster log must be contiguous for joining image: ") +
	"expectedPosition=" + std::to_string(m_logPosition) + " joinPosition=" + std::to_string(activeLog.m_image->joinPosition()),
      SOURCEINFO);
    }

  if (activeLog.m_logPosition != m_logPosition)
  {
    throw ClusterException(
      std::string("Cluster log must be contiguous for active log event: ") +
      "expectedPosition=" + std::to_string(m_logPosition) + " eventPosition=" + std::to_string(activeLog.m_logPosition),
      SOURCEINFO);
  }

  m_logAdapter->image(activeLog.m_image);
  m_logAdapter->maxLogPosition(activeLog.m_maxLogPosition);
  activeLog.m_subscription = nullptr;

  ack(NULL_VALUE);

  m_memberId = activeLog.m_memberId;
  // TODO
  //m_markFile.memberId(m_memberId);

  if (Cluster::Role::LEADER == activeLog.m_role)
  {
    for (int i = 0; i < m_sessions.size(); i++)
    {
      auto& session = m_sessions[i];

      if (m_ctx.isRespondingService() && !activeLog.m_isStartup)
      {
	session->connect(context().aeron());
      }
      
      session->resetClosing();
    }
  }
  
  role(activeLog.m_role);
  return true;
}

}}}
