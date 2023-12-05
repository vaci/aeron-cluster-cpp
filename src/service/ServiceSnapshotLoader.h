#ifndef AERON_CLUSTER_SERVICE_SERVICE_SNAPSHOT_LOADER_H
#define AERON_CLUSTER_SERVICE_SERVICE_SNAPSHOT_LOADER_H

#include "FragmentAssembler.h"

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

  inline int timeUnit() const
  {
    return m_timeUnit;
  }

  std::int32_t poll();

  ControlledPollAction onFragment(AtomicBuffer buffer, util::index_t offset, util::index_t length, Header &header);

private:
  std::shared_ptr<Image> m_image;
  ClusteredServiceAgent &m_agent;
  bool m_inSnapshot = false;
  bool m_isDone = false;
  std::int32_t m_appVersion;
  std::int32_t m_timeUnit;
  
};

}}}

#endif
