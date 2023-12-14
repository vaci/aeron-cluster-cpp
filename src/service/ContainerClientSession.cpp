#include "ContainerClientSession.h"
#include "ClusteredServiceAgent.h"
#include "ClusteredServiceConfiguration.h"

namespace aeron { namespace cluster { namespace service {

ContainerClientSession::ContainerClientSession(
  std::int64_t sessionId,
  std::int32_t responseStreamId,
  const std::string &responseChannel,
  const std::vector<char> &encodedPrincipal,
  ClusteredServiceAgent& clusteredServiceAgent) :
  m_id(sessionId),
  m_responseStreamId(responseStreamId),
  m_responseChannel(responseChannel),
  m_responsePublicationId(NULL_VALUE),
  m_encodedPrincipal(encodedPrincipal),
  m_clusteredServiceAgent(clusteredServiceAgent)
{
}

void ContainerClientSession::connect(std::shared_ptr<Aeron> aeron)
{
  try
  {
    if (m_responsePublicationId == NULL_VALUE)
    {
      m_responsePublicationId = aeron->addPublication(m_responseChannel, m_responseStreamId);

      BackoffIdleStrategy idle;
      for (int ii = 0; ii < 3; ++ii)
      {
	m_responsePublication = aeron->findExclusivePublication(m_responsePublicationId);
	if (!m_responsePublication)
	{
	  idle.idle();
	}
	else
	{
	  break;
	}
      }
    }
  }
  catch (...)
  {
    // TODO
    //clusteredServiceAgent.handleError(new ClusterException(
    //"failed to connect session response publication: " + ex.getMessage(), AeronException.Category.WARN
  }
}

void ContainerClientSession::disconnect(exception_handler_t)
{
  if (m_responsePublication != nullptr)
  {
    m_responsePublication->close();
    m_responsePublication = nullptr;
  }
  m_responsePublicationId = NULL_VALUE;
}

void ContainerClientSession::close()
{
  if (nullptr != m_clusteredServiceAgent.getClientSession(m_id))
  {
    m_clusteredServiceAgent.closeClientSession(m_id);
  }
}

std::int64_t ContainerClientSession::offer(AtomicBuffer& buffer)
{
  return m_clusteredServiceAgent.offer(buffer);
}

}}}
