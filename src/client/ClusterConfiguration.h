#ifndef AERON_CLUSTER_CLIENT_CONFIGURATION_H
#define AERON_CLUSTER_CLIENT_CONFIGURATION_H

#include <functional>
#include <string>
#include "aeron_cluster_codecs/EventCode.h"
#include "aeron_cluster_codecs/AdminResponseCode.h"
#include "aeron_cluster_codecs/AdminRequestType.h"

namespace aeron { namespace cluster { namespace client {

typedef std::function<void(
    std::int64_t sessionId,
    std::int64_t correlationId,
    AtomicBuffer buffer,
    util::index_t offset,
    util::index_t length,
    Header &header)> on_session_message_t;

inline on_session_message_t defaultSessionMessageConsumer()
{
  return
    [](std::int64_t sessionId,
       std::int64_t correlationId,
       AtomicBuffer buffer,
       util::index_t offset,
       util::index_t length,
       Header &header)
    {
    };
}

typedef std::function<void(
    std::int64_t correlationId,
    std::int64_t clusterSessionId,
    std::int64_t leadershipTermId,
    std::int32_t leaderMemberId,
    codecs::EventCode::Value code,
    const std::string &errorMessage)> on_session_event_t;

inline on_session_event_t defaultSessionEventConsumer()
{
  return
    [](std::int64_t correlationId,
       std::int64_t clusterSessionId,
       std::int64_t leadershipTermId,
       std::int32_t leaderMemberId,
       codecs::EventCode::Value code,
       const std::string &errorMessage)
    {
    };
}

typedef std::function<void(
    std::int64_t sessionId,
    std::int64_t leadershipTermId,
    std::int32_t leaderMemberId,
    const std::string &ingressEndpoints)> on_new_leader_event_t;

inline on_new_leader_event_t defaultNewLeaderEventConsumer()
{
  return
    [](std::int64_t sessionId,
       std::int64_t leadershipTermId,
       std::int32_t leaderMemberId,
       const std::string& ingressEndpoints)
    {
    };
}

typedef std::function<void(
    std::int64_t sessionId,
    std::int64_t correlationId,
    codecs::AdminRequestType::Value requestType,
    codecs::AdminResponseCode::Value code,
    const std::string &message,
    AtomicBuffer buffer,
    util::index_t offset,
    util::index_t length)> on_admin_response_t;

inline on_admin_response_t defaultAdminResponseConsumer()
{
  return
    [](std::int64_t sessionId,
       std::int64_t correlationId,
       codecs::AdminRequestType::Value requestType,
       codecs::AdminResponseCode::Value code,
       const std::string &message,
       AtomicBuffer buffer,
       util::index_t offset,
       util::index_t length)
    {
    };
}

}}}

#endif
