#ifndef INCLUDED_AERON_CLUSTER_CONTROLLED_EGRESS_LISTENER_H
#define INCLUDED_AERON_CLUSTER_CONTROLLED_EGRESS_LISTENER_H

namespace aeron { namespace cluster { namespace client {

#include <aeron.h>

class ControlledEgressListener
{
  virtual ControlledPollAction onMessage(
    int64_t clusterSessionId,
    int64_t timestamp,
    concurrent::AtomicBuffer &buffer,
    util::index_t offset,
    util::index_t length,
    Header &header);
};

}}}
#endif
