#ifndef AERON_CLUSTER_SERVICE_SERVICE_ADAPTER_H
#define AERON_CLUSTER_SERVICE_SERVICE_ADAPTER_H

#include <Aeron.h>
#include "FragmentAssembler.h"

namespace aeron { namespace cluster { namespace service {

class ClusteredServiceAgent;

class ServiceAdapter
{
public:
  explicit ServiceAdapter(
    std::shared_ptr<Subscription> subscription,
    int fragmentLimit = 1
  );

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
      return m_subscription->poll(m_fragmentHandler, m_fragmentLimit);
  }

  void onFragment(AtomicBuffer &buffer, util::index_t offset, util::index_t length, Header &header);

private:
  FragmentAssembler m_fragmentAssembler;
  fragment_handler_t m_fragmentHandler;
  std::shared_ptr<Subscription> m_subscription;
  std::shared_ptr<ClusteredServiceAgent> m_agent;
  int m_fragmentLimit;
};

}}}

#endif
