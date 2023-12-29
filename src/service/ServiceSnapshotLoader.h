#ifndef AERON_CLUSTER_SERVICE_SERVICE_SNAPSHOT_LOADER_H
#define AERON_CLUSTER_SERVICE_SERVICE_SNAPSHOT_LOADER_H

#include "ImageControlledFragmentAssembler.h"

namespace aeron { namespace cluster { namespace service {

class ClusteredServiceAgent;

class ServiceSnapshotLoader
{
public:
    static constexpr std::int32_t FRAGMENT_LIMIT = 10;

    ServiceSnapshotLoader(
	std::shared_ptr<Image> image,
	ClusteredServiceAgent &agent);

    inline bool isDone() const
    {
	return m_isDone;
    }

    inline std::int32_t appVersion() const
    {
	return m_appVersion;
    }
    
    inline int poll()
    {
	return m_image->controlledPoll(m_fragmentHandler, FRAGMENT_LIMIT);
    }

    ControlledPollAction onFragment(
	AtomicBuffer &buffer, util::index_t offset, util::index_t length, Header &header);

private:
    ImageControlledFragmentAssembler m_fragmentAssembler;
    controlled_poll_fragment_handler_t m_fragmentHandler;
    std::shared_ptr<Image> m_image;
    ClusteredServiceAgent &m_agent;
    bool m_inSnapshot = false;
    bool m_isDone = false;
    std::int32_t m_appVersion;
};

}}}

#endif
