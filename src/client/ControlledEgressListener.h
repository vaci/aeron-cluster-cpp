#ifndef INCLUDED_AERON_CLUSTER_CONTROLLED_EGRESS_LISTENER_H
#define INCLUDED_AERON_CLUSTER_CONTROLLED_EGRESS_LISTENER_H

#include <Aeron.h>
#include "aeron_cluster_codecs/EventCode.h"

namespace aeron { namespace cluster { namespace client {


class ControlledEgressListener
{
  virtual ControlledPollAction onMessage(
    int64_t clusterSessionId,
    int64_t timestamp,
    concurrent::AtomicBuffer &buffer,
    util::index_t offset,
    util::index_t length,
    Header &header) = 0;

  virtual void onSessionEvent(
    std::int64_t correlationId,
    std::int64_t clusterSessionId,
    std::int64_t leadershipTermId,
    std::int32_t leaderMemberId,
    codecs::EventCode::Value code,
    const std::string &detail)
  {}

  virtual void onNewLeader(
    std::int64_t clusterSessionId,
    std::int64_t leadershipTermId,
    std::int32_t leaderMemberId,
    const std::string &ingressEndpoints)
  {}

  virtual void onAdminResponse(
    std::int64_t clusterSessionId,
    std::int64_t correlationId,
    codecs::AdminRequestType requestType,
    codecs::AdminResponseCode responseCode,
    const std::string &message,
    AtomicBuffer payload,
    util::index_t payloadOffset,
    util::index_t payloadLength)
  {}
};

}}}
#endif
