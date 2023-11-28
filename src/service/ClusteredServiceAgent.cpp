#include "ClusteredServiceAgent.h"

namespace aeron { namespace cluster { namespace service {

std::shared_ptr<ClientSession> ClusteredServiceAgent::getClientSession(std::int64_t clusterSessionId)
{
  return nullptr;
}

bool ClusteredServiceAgent::closeClientSession(std::int64_t clusterSessionId)
{
  return false;
}

}}}
