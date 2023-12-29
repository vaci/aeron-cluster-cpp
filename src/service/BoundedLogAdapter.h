#ifndef AERON_CLUSTER_SERVICE_BOUNDED_LOG_ADAPTER_H
#define AERON_CLUSTER_SERVICE_BOUNDED_LOG_ADAPTER_H

#include <Aeron.h>
#include "ControlledFragmentAssembler.h"

namespace aeron { namespace cluster { namespace service {

class ClusteredServiceAgent;

class BoundedLogAdapter
{
public:
    BoundedLogAdapter(ClusteredServiceAgent& agent, int fragmentLimit);

    void image(std::shared_ptr<Image> image)
    {
	m_image = image;
    }

    std::shared_ptr<Image> image()
    {
	return m_image;
    }

    void maxLogPosition(std::int64_t position)
    {
	m_maxLogPosition = position;
    }

    inline bool isDone()
    {
        return
	    m_image == nullptr ||
	    m_image->position() >= m_maxLogPosition ||
	    m_image->isEndOfStream() ||
	    m_image->isClosed();
    }

    void close();

    inline int poll(std::int64_t limit)
    {
	return m_image->boundedControlledPoll(m_fragmentHandler, limit, m_fragmentLimit);
    }
  
    ControlledPollAction onFragment(
	AtomicBuffer &buffer, util::index_t offset, util::index_t length, Header &header);

private:
    ControlledPollAction onMessage(
	AtomicBuffer &buffer, util::index_t offset, util::index_t length, Header &header);

    int m_fragmentLimit;
    std::int64_t m_maxLogPosition;
    std::shared_ptr<Image> m_image;
    ClusteredServiceAgent& m_agent;
    BufferBuilder m_builder{0};
    ControlledFragmentAssembler m_fragmentAssembler;
    controlled_poll_fragment_handler_t m_fragmentHandler;
};

}}}
#endif
