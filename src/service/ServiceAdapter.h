#ifndef AERON_CLUSTER_SERVICE_SERVICE_ADAPTER_H
#define AERON_CLUSTER_SERVICE_SERVICE_ADAPTER_H

#include <Aeron.h>
#include "FragmentAssembler.h"

namespace aeron { namespace cluster { namespace service {

class ClusteredServiceAgent;

class ServiceAdapter
{
public:
  static constexpr int FRAGMENT_LIMIT = 1;

  explicit ServiceAdapter(
    std::shared_ptr<Subscription> subscription);

  ~ServiceAdapter();

  inline void agent(std::shared_ptr<ClusteredServiceAgent> agent)
  {
    m_agent = agent;
  }

  inline void close()
  {
    m_subscription->closeAndRemoveImages();
  }

  inline int poll()
  {
    std::cout << "ServiceAdapter::poll: "
	      << m_subscription->channel() << ":" << m_subscription->streamId()
	      << (m_subscription->isClosed() ? " closed " : " open ")
	      << std::endl;
    return m_subscription->poll(m_fragmentHandler, FRAGMENT_LIMIT);
  }

  void onFragment(AtomicBuffer &buffer, util::index_t offset, util::index_t length, Header &header);

private:
  FragmentAssembler m_fragmentAssembler;
  fragment_handler_t m_fragmentHandler;
  std::shared_ptr<Subscription> m_subscription;
  std::shared_ptr<ClusteredServiceAgent> m_agent;
};

}}}

#endif
