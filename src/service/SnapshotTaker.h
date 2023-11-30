#ifndef AERON_CLUSTER_SERVICE_SNAPSHOT_TAKER_H
#define AERON_CLUSTER_SERVICE_SNAPSHOT_TAKER_H

#include <Aeron.h>

#include "aeron_cluster_client/SnapshotMark.h"

namespace aeron { namespace cluster { namespace service {

using client::SnapshotMark;

class SnapshotTaker
{
public:
  SnapshotTaker(
    std::shared_ptr<ExclusivePublication> publication,
    std::shared_ptr<Aeron> aeron);

      /**
     * Mark the beginning of the encoded snapshot.
     *
     * @param snapshotTypeId   type to identify snapshot within a cluster.
     * @param logPosition      at which the snapshot was taken.
     * @param leadershipTermId at which the snapshot was taken.
     * @param snapshotIndex    so the snapshot can be sectioned.
     * @param appVersion       associated with the snapshot from {@link ClusteredServiceContainer.Context#appVersion()}.
     */
  inline void markBegin(
    std::int64_t snapshotTypeId,
    std::int64_t logPosition,
    std::int64_t leadershipTermId,
    std::int32_t snapshotIndex,
    std::int32_t appVersion)
  {
    markSnapshot(
      snapshotTypeId, logPosition, leadershipTermId, snapshotIndex, SnapshotMark::BEGIN, appVersion);
  }

        /**
     * Mark the beginning of the encoded snapshot.
     *
     * @param snapshotTypeId   type to identify snapshot within a cluster.
     * @param logPosition      at which the snapshot was taken.
     * @param leadershipTermId at which the snapshot was taken.
     * @param snapshotIndex    so the snapshot can be sectioned.
     * @param appVersion       associated with the snapshot from {@link ClusteredServiceContainer.Context#appVersion()}.
     */
  inline void markEnd(
    std::int64_t snapshotTypeId,
    std::int64_t logPosition,
    std::int64_t leadershipTermId,
    std::int32_t snapshotIndex,
    std::int32_t appVersion)
  {
    markSnapshot(
      snapshotTypeId, logPosition, leadershipTermId, snapshotIndex, SnapshotMark::END, appVersion);
  }


  void markSnapshot(
    std::int64_t snapshotTypeId,
    std::int64_t logPosition,
    std::int64_t leadershipTermId,
    std::int32_t  snapshotIndex,
    SnapshotMark::Value snapshotMark,
    std::int32_t  appVersion);

protected:
  std::shared_ptr<ExclusivePublication> m_publication;
  std::shared_ptr<Aeron> m_aeron;

  virtual void checkResultAndIdle(std::int64_t result);
};

}}}

#endif
