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
std::int32_t awaitRecoveryCounter(CountersReader &counters, std::int32_t clusterId)
{
  IdleStrategy idle;
  std::int32_t counterId = RecoveryState::findCounterId(counters, clusterId);
  while (CountersReader::NULL_COUNTER_ID == counterId)
  {
    idle.idle();
    counterId = RecoveryState::findCounterId(counters, clusterId);
  }
  return counterId;
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
  m_publicationId(publicationId),
  m_subscriptionId(subscriptionId)
{
}

std::shared_ptr<ClusteredServiceAgent> ClusteredServiceAgent::AsyncConnect::poll()
{
  if (!m_publication)
  {
    m_publication = m_aeron->findExclusivePublication(m_publicationId);
  }
  if (!m_subscription)
  {
    m_subscription = m_aeron->findSubscription(m_subscriptionId);
  }
  if (m_publication && !m_proxy)
  {
    m_proxy = std::make_shared<ConsensusModuleProxy>(m_publication);
  }
  if (m_subscription && !m_serviceAdapter)
  {
    m_serviceAdapter = std::make_shared<ServiceAdapter>(m_subscription);
  }
  if (0 == m_step && m_proxy && m_serviceAdapter)
  {
    if (!m_proxy->publication()->isConnected())
    {
      return {};
    }

    m_step = 1;
  }

  if (m_step == 1)
  {
    auto agent = std::make_shared<ClusteredServiceAgent>(m_ctx, m_proxy, m_serviceAdapter);
    m_serviceAdapter->agent(agent);
    return agent;
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

void ClusteredServiceAgent::loadState(std::shared_ptr<Image> image, std::shared_ptr<AeronArchive> archive)
{
  BackoffIdleStrategy idle;

  ServiceSnapshotLoader snapshotLoader(image, *this);
  while (true)
  {
    std::int32_t fragments = snapshotLoader.poll();
    if (snapshotLoader.isDone())
    {
      break;
    }

    if (fragments == 0)
    {
      archive->checkForErrorResponse();
      if (image->isClosed())
      {
	throw ClusterException("snapshot ended unexpectedly: ", SOURCEINFO);
      }
    }

    idle.idle(fragments);
  }

  // TODO
  //std::int32_t appVersion = snapshotLoader.appVersion();
  //if (!m_ctx.appVersionValidator().isVersionCompatible(ctx.appVersion(), appVersion))
  //{
  //  throw ClusterException(
  //    "incompatible app version: " + SemanticVersion.toString(ctx.appVersion()) +
  //    " snapshot=" + SemanticVersion.toString(appVersion));
  //}

  m_timeUnit = snapshotLoader.timeUnit();
}

void ClusteredServiceAgent::loadSnapshot(std::int64_t recordingId)
{
  BackoffIdleStrategy idle;

  auto archive = AeronArchive::connect(m_ctx.archiveContext());

  auto& channel = m_ctx.replayChannel();
  std::int32_t streamId = m_ctx.replayStreamId();
  std::int32_t  sessionId = (int)archive->startReplay(recordingId, 0, NULL_VALUE, channel, streamId);

  auto replaySessionChannel = ChannelUri::addSessionId(channel, sessionId);

  std::int64_t subscriptionRegistrationId = m_aeron->addSubscription(replaySessionChannel, streamId);

  auto subscription = m_aeron->findSubscription(subscriptionRegistrationId);
  while (!subscription)
  {
    idle.idle();
    subscription = m_aeron->findSubscription(subscriptionRegistrationId);
  }

  auto image = subscription->imageBySessionId(sessionId);
  while (!image)
  {
    idle.idle();
    image = subscription->imageBySessionId(sessionId);
  }

  loadState(image, archive);
  
  m_service->onStart(*this, image);
}

void ClusteredServiceAgent::recoverState(CountersReader& counters)
{
  std::int32_t recoveryCounterId = awaitRecoveryCounter(counters, m_ctx.clusterId());
  auto logPosition = RecoveryState::getLogPosition(counters, recoveryCounterId);
  m_clusterTime = RecoveryState::getTimestamp(counters, recoveryCounterId);
  std::int64_t leadershipTermId = RecoveryState::getLeadershipTermId(counters, recoveryCounterId);

  m_sessionMessageHeader.leadershipTermId(leadershipTermId);
  m_activeLifecycleCallbackName = "onStart";
  try
  {
    if (NULL_VALUE != leadershipTermId)
    {
      auto snapshotRecordingId = RecoveryState::getSnapshotRecordingId(counters, recoveryCounterId, m_serviceId);
      loadSnapshot(snapshotRecordingId);
    }
    else
    {
      m_service->onStart(*this, nullptr);
    }
  }
  catch (...)
  {
    m_activeLifecycleCallbackName = nullptr;
    throw;
  }

  m_activeLifecycleCallbackName = nullptr;

  std::int64_t id = m_ackId++;

  BackoffIdleStrategy idle;
  while (!m_consensusModuleProxy->ack(logPosition, m_clusterTime, id, m_aeron->clientId(), m_serviceId))
  {
    idle.idle();
  }
}


void ClusteredServiceAgent::onStart()
{
  //closeHandlerRegistrationId = aeron.addCloseHandler(this::abort);
  m_aeron->addUnavailableCounterHandler(unavailableCounterHandler(*this));
  auto& counters = m_aeron->countersReader();

  m_commitPosition = awaitCounter(counters, Configuration::COMMIT_POSITION_TYPE_ID, m_ctx.clusterId());

  recoverState(counters);
  //dutyCycleTracker.update(nanoClock.nanoTime());
  m_isServiceActive = true;
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
  Context &ctx,
  std::shared_ptr<ConsensusModuleProxy> proxy,
  std::shared_ptr<ServiceAdapter> serviceAdapter) :
  m_ctx(ctx),
  m_aeron(ctx.aeron()),
  m_nanoClock(systemNanoClock),
  m_epochClock(currentTimeMillis),
  m_service(ctx.clusteredService()),
  m_serviceId(ctx.serviceId()),
  m_subscriptionAlias(std::string("log-sc-") + std::to_string(ctx.serviceId())),
  m_consensusModuleProxy(proxy),
  m_serviceAdapter(serviceAdapter)
{
}

std::shared_ptr<ClientSession> ClusteredServiceAgent::getClientSession(std::int64_t clusterSessionId)
{
  return nullptr;
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

  if (m_consensusModuleProxy->closeSession(clusterSessionId))
  {
    clientSession->second->markClosing();
    return true;
  }
  
  return false;
}

std::int64_t ClusteredServiceAgent::offer(AtomicBuffer& buffer)
{
  checkForValidInvocation();
  m_sessionMessageHeader.clusterSessionId(context().serviceId());

  AtomicBuffer headerBuffer(
    reinterpret_cast<std::uint8_t*>(m_sessionMessageHeader.buffer()),
    m_sessionMessageHeader.bufferLength());
  return m_consensusModuleProxy->offer(headerBuffer, buffer);
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


std::int64_t ClusteredServiceAgent::onTakeSnapshot(std::int64_t logPosition, std::int64_t leadershipTermId)
{
  try
  {
    auto archive = AeronArchive::connect(m_ctx.archiveContext());

    auto publicationId = m_aeron->addExclusivePublication(
      m_ctx.snapshotChannel(), m_ctx.snapshotStreamId());

    auto publication = m_aeron->findExclusivePublication(publicationId);
    {
      BackoffIdleStrategy idle;
      while (!publication)
      {
	idle.idle();
	publication = m_aeron->findExclusivePublication(publicationId);
      }
    }

    auto channel = ChannelUri::addSessionId(m_ctx.snapshotChannel(), publication->sessionId());
    archive->startRecording(channel, m_ctx.snapshotStreamId(), AeronArchive::LOCAL, true);
    auto& counters = m_aeron->countersReader();
    std::int32_t counterId = awaitRecordingCounter(publication->sessionId(), counters, archive);
    std::int64_t recordingId = archive::client::RecordingPos::getRecordingId(counters, counterId);

    snapshotState(publication, logPosition, leadershipTermId);
    checkForClockTick(m_nanoClock());
    archive->checkForErrorResponse();
    m_service->onTakeSnapshot(publication);
    awaitRecordingComplete(recordingId, publication->position(), counters, counterId, archive);

    return recordingId;
  }
  catch (const archive::client::ArchiveException &ex)
  {
    // TODO
    throw ex;
  }
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


}}}
