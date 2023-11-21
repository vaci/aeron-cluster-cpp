#ifndef AERON_CLUSTER_EGRESS_ADAPTER_H
#define AERON_CLUSTER_EGRESS_ADAPTER_H

#include "FragmentAssembler.h"
#include "aeron_cluster_client/EventCode.h"
#include "aeron_cluster_client/AdminResponseCode.h"
#include "aeron_cluster_client/AdminRequestType.h"

namespace aeron { namespace cluster { namespace client {

typedef std::function<void(
    std::int64_t sessionId,
    std::int64_t correlationId,
    AtomicBuffer buffer,
    util::index_t offset,
    util::index_t length,
    Header &header)> on_session_message_t;

typedef std::function<void(
    std::int64_t correlationId,
    std::int64_t clusterSessionId,
    std::int64_t leadershipTermId,
    std::int32_t leaderMemberId,
    EventCode::Value code,
    const std::string &errorMessage)> on_session_event_t;

typedef std::function<void(
    std::int64_t sessionId,
    std::int64_t leadershipTermId,
    std::int32_t leaderMemberId,
    const std::string& ingressEndpoints)> on_new_leader_event_t;

typedef std::function<void(
    std::int64_t sessionId,
    std::int64_t correlationId,
    AdminRequestType::Value requestType,
    AdminResponseCode::Value code,
    const std::string& message,
    AtomicBuffer buffer,
    util::index_t offset,
    util::index_t length)> on_admin_response_t;

class EgressAdapter
{
public:
  EgressAdapter(
    const on_session_message_t &onSessionMessage,
    const on_session_event_t &onSessionEvent,
    const on_new_leader_event_t &onNewLeaderEvent,
    const on_admin_response_t &onAdminResponse,
    int64_t clusterSessionId,
    std::shared_ptr<Subscription> subscription,
    int fragmentLimit = 10);

  inline int poll()
  {
    return m_subscription->poll(m_fragmentHandler, m_fragmentLimit);
  }

  void onFragment(AtomicBuffer buffer, util::index_t offset, util::index_t length, Header &header);

private:
  FragmentAssembler m_fragmentAssembler;
  fragment_handler_t m_fragmentHandler;
  int64_t m_clusterSessionId;
  std::shared_ptr<Subscription> m_subscription;
  on_session_message_t m_onSessionMessage;
  on_session_event_t m_onSessionEvent;
  on_new_leader_event_t m_onNewLeaderEvent;
  on_admin_response_t m_onAdminResponse;
  const int m_fragmentLimit;
};

}}}

#endif
