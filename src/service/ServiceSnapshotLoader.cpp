#include "ServiceSnapshotLoader.h"

namespace aeron { namespace cluster { namespace service {

namespace {

static controlled_poll_fragment_handler_t fragmentHandler(ServiceSnapshotLoader &loader)
{
  return
    [&](AtomicBuffer &buffer, util::index_t offset, util::index_t length, Header &header)
    {
      return loader.onFragment(buffer, offset, length, header);
    };
}

}

ServiceSnapshotLoader::ServiceSnapshotLoader(
  std::shared_ptr<Image> image,
  ClusteredServiceAgent &agent) :
  m_image(image),
  m_agent(agent)
{
}

std::int32_t ServiceSnapshotLoader::poll()
{
  return m_image->controlledPoll(fragmentHandler(*this), FRAGMENT_LIMIT);
}


ControlledPollAction ServiceSnapshotLoader::onFragment(AtomicBuffer buffer, util::index_t offset, util::index_t length, Header &header)
{
  // TODO
  return ControlledPollAction::BREAK;
}

}}}
