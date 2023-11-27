#ifndef AERON_CLUSTER_EGRESS_LISTENER_H
#define AERON_CLUSTER_EGRESS_LISTENER_H

namespace aeron { namespace cluster { namespace client {

struct EgressListener
{
    /**
     * Message returned from the clustered service.
     *
     * @param clusterSessionId to which the message belongs.
     * @param timestamp        at which the correlated ingress was sequenced in the cluster.
     * @param buffer           containing the message.
     * @param offset           at which the message begins.
     * @param length           of the message in bytes.
     * @param header           Aeron header associated with the message fragment.
     */
  virtual void onMessage(
    std::int64_t clusterSessionId,
    std::int64_t timestamp,
    AtomicBuffer buffer,
    util::index_t offset,
    util::index_t length,
    Header &header)
  {}

  
  /**
   * Session event emitted from the cluster which after connect can indicate an error or session close.
   *
   * @param correlationId    associated with the cluster ingress.
   * @param clusterSessionId to which the event belongs.
   * @param leadershipTermId for identifying the active term of leadership
   * @param leaderMemberId   identity of the active leader.
   * @param code             to indicate the type of event.
   * @param detail           Textual detail to explain the event.
   */
  virtual void onSessionEvent(
    std::int64_t correlationId,
    std::int64_t clusterSessionId,
    std::int64_t leadershipTermId,
    std::int32_t leaderMemberId,
    EventCode code,
    const std::string &detail)
  {
  }

  /**
   * Event indicating a new leader has been elected.
   *
   * @param clusterSessionId to which the event belongs.
   * @param leadershipTermId for identifying the active term of leadership
   * @param leaderMemberId   identity of the active leader.
   * @param ingressEndpoints for connecting to the cluster which can be updated due to dynamic membership.
   */
  virtual void onNewLeader(
    std::int64_t clusterSessionId,
    std::int64_t leadershipTermId,
    std::int32_t leaderMemberId,
    const std::string &ingressEndpoints)
  {
  }

  /**
   * Message returned in response to an admin request.
   *
   * @param clusterSessionId to which the response belongs.
   * @param correlationId    of the admin request.
   * @param requestType      of the admin request.
   * @param responseCode     describing the response.
   * @param message          describing the response (e.g. error message).
   * @param payload          delivered with the response, can be empty.
   * @param payloadOffset    into the payload buffer.
   * @param payloadLength    of the payload.
   */
  virtual void onAdminResponse(
    std::int64_t clusterSessionId,
    std::int64_t correlationId,
    AdminRequestType requestType,
    AdminResponseCode responseCode,
    const std::string &message,
    AtomicBuffer payload,
    std::int32_t payloadOffset,
    std::int32_t payloadLength)
  {
  }
};

}}}

#endif // AERON_CLUSTER_EGRESS_LISTENER_H
