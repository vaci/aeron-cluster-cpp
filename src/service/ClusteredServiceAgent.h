#ifndef AERON_CLUSTER_SERVICE_CLUSTERED_SERVICE_AGENT_H
#define AERON_CLUSTER_SERVICE_CLUSTERED_SERVICE_AGENT_H

#include <Aeron.h>
#include <client/AeronArchive.h>
#include <cstdint>
#include "ContainerClientSession.h"
#include "ClientSession.h"
#include "ConsensusModuleProxy.h"
#include "Cluster.h"
#include "ServiceAdapter.h"
#include "ClusteredService.h"
#include "ClusteredServiceConfiguration.h"
#include "aeron_cluster_client/SessionMessageHeader.h"

#include <unordered_map>
#include <vector>

namespace aeron { namespace cluster { namespace service {

using AeronArchive = archive::client::AeronArchive;

using SessionMessageHeader = client::SessionMessageHeader;

class ClusteredServiceAgent :
    public Cluster
{
public:
  explicit ClusteredServiceAgent(
    Context &context,
     std::shared_ptr<ConsensusModuleProxy> proxy,
     std::shared_ptr<ServiceAdapter> serviceadapter);
  std::shared_ptr<ClientSession> getClientSession(std::int64_t clusterSessionId) override;
  bool closeClientSession(std::int64_t clusterSessionId) override;

  void onTimerEvent(std::int64_t logPosition, std::int64_t correlationId, std::int64_t timestamp);

  std::int64_t offer(AtomicBuffer& buffer);

  inline bool cancelTimer(std::int64_t correlationId)
  {
    checkForValidInvocation();
    return m_consensusModuleProxy->cancelTimer(correlationId);
  }

  inline Context &context() {
    return m_ctx;
  }

  inline Cluster::Role role() const
  {
    return m_role;
  }

  void onStart();

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
    std::int64_t m_publicationId;
    std::shared_ptr<ExclusivePublication> m_publication;
    std::int64_t m_subscriptionId;
    std::shared_ptr<Subscription> m_subscription;
    std::shared_ptr<ConsensusModuleProxy> m_proxy;
    std::shared_ptr<ServiceAdapter> m_serviceAdapter;
    int m_step = 0;
  };

  static std::shared_ptr<AsyncConnect> asyncConnect(Context &ctx);

private:
  Context &m_ctx;
  std::shared_ptr<Aeron> m_aeron;
  nano_clock_t m_nanoClock;
  epoch_clock_t m_epochClock;
  SessionMessageHeader m_sessionMessageHeader;
  std::int64_t m_clusterTime = NULL_VALUE;
  bool m_isAbort;
  bool m_isServiceActive = false;
  std::int64_t m_lastSlowTickNs;
  std::int32_t m_serviceId;
  std::int32_t m_memberId = NULL_VALUE;
  std::int64_t m_ackId = 0;
  std::int64_t m_terminationPosition = archive::client::NULL_POSITION;
  std::int64_t m_logPosition = archive::client::NULL_POSITION;
  std::shared_ptr<ClusteredService> m_service;
  std::shared_ptr<ConsensusModuleProxy> m_consensusModuleProxy;
  std::shared_ptr<ServiceAdapter> m_serviceAdapter;
  std::unordered_map<std::int64_t, std::shared_ptr<ContainerClientSession>> m_sessionByIdMap;
  std::vector<std::shared_ptr<ContainerClientSession>> m_sessions;
  bool m_isBackgroundInvocation;
  std::string m_subscriptionAlias;
  const char *m_activeLifecycleCallbackName = nullptr;

  Cluster::Role m_role = Cluster::Role::FOLLOWER;
  std::int64_t m_requestedAckPosition = archive::client::NULL_POSITION;
  int m_timeUnit;

  std::shared_ptr<Counter> m_commitPosition;
  std::int64_t m_markFileUpdateDeadlineMs = 1;

  void role(Cluster::Role newRole);
  void checkForValidInvocation();
  void recoverState(CountersReader& counters);
  void loadSnapshot(std::int64_t recordingId);
  void loadState(std::shared_ptr<Image> image, std::shared_ptr<AeronArchive> archive);
  void disconnectEgress();
  void snapshotState(
    std::shared_ptr<ExclusivePublication>,
    std::int64_t logPosition,
    std::int64_t leadershipTermId);
		     

  std::int64_t onTakeSnapshot(std::int64_t logPosition, std::int64_t leadershipTermId);
  bool checkForClockTick(std::int64_t nowNs);
};

}}}

#endif
