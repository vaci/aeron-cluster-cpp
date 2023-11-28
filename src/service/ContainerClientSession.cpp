#include "ContainerClientSession.h"

namespace aeron { namespace cluster { namespace service {

ContainerClientSession::ContainerClientSession(
  std::int64_t sessionId,
  std::int32_t responseStreamId,
  const std::string &responseChannel,
  //final byte[] encodedPrincipal,
  std::shared_ptr<ClusteredServiceAgent> clusteredServiceAgent) :
  m_id(sessionId),
  m_responseStreamId(responseStreamId),
  m_responseChannel(responseChannel),
  m_clusteredServiceAgent(clusteredServiceAgent)
{
}

}}}
