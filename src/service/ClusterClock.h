#ifndef AERON_CLUSTER_SERVICE_CLUSTER_CLOCK_H
#define AERON_CLUSTER_SERVICE_CLUSTER_CLOCK_H

#include <Aeron.h>

namespace aeron { namespace cluster { namespace service {

class ClusterClock
{
  // the time in nanos
  virtual std::int64_t time()  = 0;

};

}}}
#endif
