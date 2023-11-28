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
  m_responseRegistration(NULL_VALUE),
  m_clusteredServiceAgent(clusteredServiceAgent)
{
}

void ContainerClientSession::connect(std::shared_ptr<Aeron> aeron)
{
  try
  {
    if (NULL_VALUE == m_responseRegistration)
    {
      m_responseRegistration = aeron->addPublication(m_responseChannel, m_responseStreamId);
    }
  }
  catch (...)
  {
    // clusteredServiceAgent.handleError(new ClusterException(
    //				  "failed to connect session response publication: " + ex.getMessage(), AeronException.Category.WARN));
  }
}

}}}
