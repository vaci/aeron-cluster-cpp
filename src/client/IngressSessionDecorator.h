#ifndef AERON_CLUSTER_INGRESS_SESSION_DECORATOR_H
#define AERON_CLUSTER_INGRESS_SESSION_DECORATOR_H

#include "Aeron.h"
#include "aeron_cluster_client/MessageHeader.h"
#include "aeron_cluster_client/SessionMessageHeader.h"

namespace aeron { namespace cluster { namespace client {

class IngressSessionDecorator
{
public:
  /**
   * Construct a new ingress session header wrapper that defaults all fields to {@link Aeron#NULL_VALUE}.
   */
  IngressSessionDecorator()
    : IngressSessionDecorator(NULL_VALUE, NULL_VALUE)
  {}

  /**
   * Construct a new session header wrapper.
   *
   * @param clusterSessionId that has been allocated by the cluster.
   * @param leadershipTermId of the current leader.
   */
  IngressSessionDecorator(std::int64_t clusterSessionId, std::uint64_t leadershipTermId);

  /**
   * Reset the cluster session id in the header.
   *
   * @param clusterSessionId to be set in the header.
   * @return this for a fluent API.
   */
  void clusterSessionId(std::int64_t clusterSessionId)
  {
    m_message.clusterSessionId(clusterSessionId);
  }

  /**
   * Reset the leadership term id in the header.
   *
   * @param leadershipTermId to be set in the header.
   * @return this for a fluent API.
   */
  void leadershipTermId(std::int64_t leadershipTermId)
  {
    m_message.leadershipTermId(leadershipTermId);
  }

private:
  static constexpr std::uint64_t HEADER_LENGTH =
    MessageHeader::encodedLength() + SessionMessageHeader::sbeBlockLength();
  
  SessionMessageHeader m_message;
  char m_headerBuffer[HEADER_LENGTH];
};

}}}

#endif // AERON_CLUSTER_INGRESS_SESSION_DECORATOR_H
