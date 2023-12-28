#include "ConsensusModuleProxy.h"
#include "client/ClusterException.h"

namespace aeron { namespace cluster { namespace service {

ConsensusModuleProxy::ConsensusModuleProxy(std::shared_ptr<ExclusivePublication> publication) :
  m_publication(std::move(publication))
{
}

void ConsensusModuleProxy::checkResult(std::int64_t result)
{
  using client::ClusterException;
  
  if (result == NOT_CONNECTED ||
      result == PUBLICATION_CLOSED ||
      result == MAX_POSITION_EXCEEDED)
  {
    throw ClusterException(
      std::string("unexpected publication state: ") +
      std::to_string(result), SOURCEINFO);
  }
}

}}}
