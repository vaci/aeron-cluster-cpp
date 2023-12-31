#ifndef AERON_CLUSTER_SERVICE_CONTAINER_CLIENT_SESSION_H
#define AERON_CLUSTER_SERVICE_CONTAINER_CLIENT_SESSION_H

#include <Aeron.h>
#include <cstdint>
#include "ClientSession.h"

namespace aeron { namespace cluster { namespace service {

class ClusteredServiceAgent;

class ContainerClientSession
  : public ClientSession
{
public:
  ContainerClientSession(
    std::int64_t sessionId,
    std::int32_t responseStreamId,
    const std::string &responseChannel,
    const std::vector<char> &encodedPrincipal,
    ClusteredServiceAgent &clusteredServiceAgent);

  inline std::int64_t id() const override
  {
    return m_id;
  }

  inline std::int32_t responseStreamId() const override
  {
    return m_responseStreamId;
  }

  inline const std::string &responseChannel() const override
  {
    return m_responseChannel;
  }

  const std::vector<char> &encodedPrincipal() const override
  {
      return m_encodedPrincipal;
  }

  void connect(std::shared_ptr<Aeron> aeron);

  void close();

  inline void markClosing()
  {
    m_isClosing = true;
  }

  inline void resetClosing()
  {
    m_isClosing = false;
  }

  inline bool isClosing() const
  {
    return m_isClosing;
  }

  void disconnect(exception_handler_t = nullptr);

  std::int64_t offer(AtomicBuffer& buffer);

  /*

    public long offer(final DirectBufferVector[] vectors)
    {
        return clusteredServiceAgent.offer(id, responsePublication, vectors);
    }

    public long tryClaim(final int length, final BufferClaim bufferClaim)
    {
        return clusteredServiceAgent.tryClaim(id, responsePublication, length, bufferClaim);
    }
    */

private: 
  std::int64_t m_id;
  std::int32_t m_responseStreamId;
  std::string m_responseChannel;
  std::vector<char> m_encodedPrincipal;
  ClusteredServiceAgent& m_clusteredServiceAgent;

  std::int64_t m_responsePublicationId;;
  std::shared_ptr<ExclusivePublication> m_responsePublication;
  bool m_isClosing;
};

}}}

#endif
