#include "ServiceAdapter.h"
#include "ClusteredServiceAgent.h"

namespace aeron { namespace cluster { namespace service {

namespace {

static aeron::fragment_handler_t fragmentHandler(ServiceAdapter &adapter)
{
  return
    [&](AtomicBuffer &buffer, util::index_t offset, util::index_t length, Header &header)
    {
      adapter.onFragment(buffer, offset, length, header);
    };
}

}

ServiceAdapter::ServiceAdapter(
  std::shared_ptr<Subscription> subscription) :
  m_fragmentAssembler(fragmentHandler(*this)),
  m_fragmentHandler(m_fragmentAssembler.handler()),
  m_subscription(subscription)
{
}

ServiceAdapter::~ServiceAdapter()
{
}

void ServiceAdapter::onFragment(AtomicBuffer &buffer, util::index_t offset, util::index_t length, Header &header)
{
}

}}}
