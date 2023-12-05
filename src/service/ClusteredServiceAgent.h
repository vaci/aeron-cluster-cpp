#ifndef AERON_CLUSTER_SERVICE_CLUSTERED_SERVICE_AGENT_H
#define AERON_CLUSTER_SERVICE_CLUSTERED_SERVICE_AGENT_H

#include <Aeron.h>
#include <client/AeronArchive.h>
#include <cstdint>
#include "BoundedLogAdapter.h"
#include "ContainerClientSession.h"
#include "ClientSession.h"
#include "ConsensusModuleProxy.h"
#include "Cluster.h"
#include "ServiceAdapter.h"
#include "ClusteredService.h"
#include "ClusteredServiceConfiguration.h"
#include "aeron_cluster_client/SessionMessageHeader.h"
#include "aeron_cluster_client/ClusterAction.h"

#include <unordered_map>
#include <vector>
#include <queue>

namespace aeron { namespace cluster { namespace service {

using AeronArchive = archive::client::AeronArchive;

using client::SessionMessageHeader;
using client::ClusterAction;

class ServiceSnapshotLoader;
class ClusteredServiceAgent;


class ClusteredServiceAgent :
    public Cluster
{
public:
  enum class CurrentAction {
    NONE,
    TAKING_SNAPSHOT
  };

  static constexpr auto NULL_POSITION = archive::client::NULL_POSITION;
  explicit ClusteredServiceAgent(Context &context);

  std::shared_ptr<ClientSession> getClientSession(std::int64_t clusterSessionId) override;
  bool closeClientSession(std::int64_t clusterSessionId) override;

  void onTimerEvent(std::int64_t logPosition, std::int64_t correlationId, std::int64_t timestamp);

  std::int64_t offer(AtomicBuffer& buffer);

  inline void addSession(
    std::int64_t clusterSessionId,
    std::int32_t responseStreamId,
    const std::string &responseChannel,
    const std::vector<char> &encodedPrincipal)
  {
    auto session = std::make_unique<ContainerClientSession>(
      clusterSessionId, responseStreamId, responseChannel, encodedPrincipal, *this);
    addSession(std::move(session));
  }

  void addSession(std::unique_ptr<ContainerClientSession> session);

  inline bool cancelTimer(std::int64_t correlationId)
  {
    checkForValidInvocation();
    return m_proxy->cancelTimer(correlationId);
  }

  inline Context &context() {
    return m_ctx;
  }

  inline Cluster::Role role() const
  {
    return m_role;
  }

  void onUnavailableCounter(CountersReader &countersReader, std::int64_t registrationId, std::int32_t counterId);

  struct AsyncConnect
  {
    AsyncConnect(
      Context &,
      std::int64_t publicationId,
      std::int64_t subscriptionId);

    std::shared_ptr<ClusteredServiceAgent> poll();

  private:
    Context &m_ctx;
    std::shared_ptr<Aeron> m_aeron;
    std::shared_ptr<AeronArchive::AsyncConnect> m_aeronArchiveConnect;
    std::int64_t m_publicationId;
    std::shared_ptr<ExclusivePublication> m_publication;
    std::int64_t m_subscriptionId;
    std::shared_ptr<Subscription> m_subscription;
    std::shared_ptr<Counter> m_recoveryCounter;
    std::int64_t m_clusterTime = NULL_VALUE;
    std::int64_t m_leadershipTermId = NULL_VALUE;
    std::int64_t m_snapshotRecordingId = NULL_VALUE;
    std::int64_t m_snapshotSubscriptionId;
    std::int64_t m_snapshotSessionId;
    std::shared_ptr<Subscription> m_snapshotSubscription;
    std::shared_ptr<Image> m_snapshotImage;
    std::unique_ptr<ServiceSnapshotLoader> m_snapshotLoader;
    std::shared_ptr<ClusteredServiceAgent> m_agent;
    int m_step = 0;
  };

  static std::shared_ptr<AsyncConnect> asyncConnect(Context &ctx);

private:
  Context &m_ctx;
  std::shared_ptr<Aeron> m_aeron;
  std::shared_ptr<AeronArchive> m_archive = nullptr;
  nano_clock_t m_nanoClock;
  epoch_clock_t m_epochClock;
  std::int64_t m_clusterTime = NULL_VALUE;
  bool m_isAbort;
  bool m_isServiceActive = false;
  std::int64_t m_lastSlowTickNs;
  std::int32_t m_serviceId;
  std::int32_t m_memberId = NULL_VALUE;
  std::int64_t m_ackId = 0;
  std::int64_t m_terminationPosition = NULL_POSITION;
  std::int64_t m_logPosition = NULL_POSITION;
  std::int64_t m_leadershipTermId = NULL_VALUE;
  std::shared_ptr<ClusteredService> m_service;
  std::shared_ptr<ConsensusModuleProxy> m_proxy;
  std::shared_ptr<ServiceAdapter> m_serviceAdapter;
  std::unordered_map<std::int64_t, ContainerClientSession*> m_sessionByIdMap;
  std::vector<std::unique_ptr<ContainerClientSession>> m_sessions;
  bool m_isBackgroundInvocation;
  std::string m_subscriptionAlias;
  const char *m_activeLifecycleCallbackName = nullptr;

  Cluster::Role m_role = Cluster::Role::FOLLOWER;
  std::int64_t m_requestedAckPosition = NULL_POSITION;
  int m_timeUnit;

  std::shared_ptr<Counter> m_commitPosition;
  std::int64_t m_markFileUpdateDeadlineMs = 1;
  std::int32_t m_standbySnapshotFlags;
  CurrentAction m_currentAction = CurrentAction::NONE;

  std::unique_ptr<BoundedLogAdapter> m_logAdapter;

  struct SnapshotState
  {
    SnapshotState(ClusteredServiceAgent& agent)
      : m_agent(agent)
    {
    }
 
    ClusteredServiceAgent& m_agent;
    
    std::int64_t m_publicationId;
    std::shared_ptr<ExclusivePublication> m_publication;
    std::int32_t m_recordingCounterId = CountersReader::NULL_COUNTER_ID;
    std::int64_t m_recordingId = NULL_VALUE;
    bool m_snapshotComplete = false;

    bool doWork();
  };

  std::unique_ptr<SnapshotState> m_currentSnapshot;

  void role(Cluster::Role newRole);
  void checkForValidInvocation();
  void disconnectEgress();
  void snapshotState(
    std::shared_ptr<ExclusivePublication>,
    std::int64_t logPosition,
    std::int64_t leadershipTermId);
		     

  bool checkForClockTick(std::int64_t nowNs);
  void executeAction(
    ClusterAction::Value action,
    std::int64_t logPosition,
    std::int64_t leadershipTermId,
    std::int32_t flags);

  void onNewLeadershipTermEvent(
  std::int64_t leadershipTermId,
  std::int64_t logPosition,
  std::int64_t timestamp,
  std::int64_t termBaseLogPosition,
  std::int32_t leaderMemberId,
  std::int32_t logSessionId,
  std::int32_t appVersion);

  void onSessionOpen(
    std::int64_t leadershipTermId,
    std::int64_t logPosition,
    std::int64_t clusterSessionId,
    std::int64_t timestamp,
    std::int32_t responseStreamId,
    const std::string &responseChannel,
    const std::vector<char> &encodedPrincipal);

  inline void onSessionMessage(
    std::int64_t logPosition,
    std::int64_t clusterSessionId,
    std::int64_t timestamp,
    AtomicBuffer &buffer,
    util::index_t offset,
    util::index_t length,
    Header &header)
  {
    m_logPosition = logPosition;
    m_clusterTime = timestamp;
    auto found = m_sessionByIdMap.find(clusterSessionId);
    m_service->onSessionMessage(*found->second, timestamp, buffer, offset, length, header);
  }

  void terminate(bool expected);
  inline bool shouldSnapshot(std::int32_t flags)
  {
    return Configuration::CLUSTER_ACTION_FLAGS_DEFAULT == flags
      || 0 != (flags & m_standbySnapshotFlags);
  }

  friend class AsyncConnect;
  friend class BoundedLogAdapter;

  struct ActiveLogEvent
  {
    std::int64_t m_logPosition;
    std::int64_t m_maxLogPosition;
    std::int32_t m_memberId;
    std::int32_t m_sessionId;
    std::int32_t m_streamId;
    bool m_isStartup;
    Role m_role;
    std::string m_channel;
  };

  struct Ack
  {
    std::int64_t m_logPosition;
    std::int64_t m_timestamp;
    std::int64_t m_ackId;
    std::int64_t m_relevantId;
    std::int32_t m_serviceId;
  };

  std::queue<Ack> m_ackQueue;
  std::unique_ptr<ActiveLogEvent> m_activeLogEvent;

  void onTakeSnapshot(std::int64_t logPosition, std::int64_t leadershipTermId);
  void doWork();
  void processAckQueue();
  int pollServiceAdapter();
  void ack(std::int64_t relevantId);
  void ackDone(std::int64_t relevantId);
};

inline void ClusteredServiceAgent::executeAction(
  ClusterAction::Value action,
  std::int64_t logPosition,
  std::int64_t leadershipTermId,
  std::int32_t flags)
{
  if (ClusterAction::Value::SNAPSHOT == action && shouldSnapshot(flags))
  {
    onTakeSnapshot(logPosition, leadershipTermId);
  }
}

}}}

#endif
