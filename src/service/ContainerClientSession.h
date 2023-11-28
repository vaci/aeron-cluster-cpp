#ifndef AERON_CLUSTER_SERVICE_CONTAINER_CLIENT_SESSION_H
#define AERON_CLUSTER_SERVICE_CONTAINER_CLIENT_SESSION_H

#include <Aeron.h>
#include <cstdint>
#include "ClientSession.h"
#include "ClusteredServiceAgent.h"

namespace aeron { namespace cluster { namespace service {

class ContainerClientSession
  : public ClientSession
{
 public:
    ContainerClientSession(
      std::int64_t sessionId,
      std::int32_t responseStreamId,
      const std::string &responseChannel,
      //final byte[] encodedPrincipal,
      std::shared_ptr<ClusteredServiceAgent> clusteredServiceAgent);

    inline std::int64_t id() const
    {
      return m_id;
    }

    inline std::int32_t responseStreamId() const
    {
      return m_responseStreamId;
    }

    inline const std::string &responseChannel() const
    {
        return m_responseChannel;
    }

    //public byte[] encodedPrincipal()
    //{
    //    return encodedPrincipal;
    //}

    void close()
    {
      if (nullptr != m_clusteredServiceAgent->getClientSession(m_id))
        {
	  m_clusteredServiceAgent->closeClientSession(m_id);
        }
    }

    bool isClosing()
    {
        return m_isClosing;
    }

 private: 
    std::int64_t m_id;
    std::int32_t m_responseStreamId;
    std::string m_responseChannel;
    //private final byte[] encodedPrincipal;

    std::shared_ptr<ClusteredServiceAgent> m_clusteredServiceAgent;
    std::shared_ptr<ExclusivePublication> m_responsePublication;
    bool m_isClosing;
};

}}}

#endif
