#ifndef AERON_CLUSTER_SERVICE_CLUSTER_COUNTERS_H
#define AERON_CLUSTER_SERVICE_CLUSTER_COUNTERS_H

#include <Aeron.h>

namespace aeron { namespace cluster { namespace service {

namespace ClusterCounters {

std::int64_t allocate(
  std::shared_ptr<Aeron> aeron,
  const std::string &name,
  std::int32_t typeId,
  std::int32_t clusterId);

std::shared_ptr<Counter> find(CountersReader& counters, std::int32_t typeId, std::int32_t clusterId);

}

}}}

#endif
