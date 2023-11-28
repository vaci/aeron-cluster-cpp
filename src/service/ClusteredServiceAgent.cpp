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

std::int64_t ClusteredServiceAgent::offer(AtomicBuffer& buffer)
{
  // TODO
  //checkForValidInvocation();
  m_header.clusterSessionId(context().serviceId());
  std::array<AtomicBuffer, 2> buffers;
  buffers[0] = m_header;
  buffers[1] = buffer;
  return m_consensusModuleProxy->offer(buffers);
}
}}}
