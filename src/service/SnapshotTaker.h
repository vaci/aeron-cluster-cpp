#ifndef AERON_CLUSTER_SERVICE_SNAPSHOT_TAKER_H
#define AERON_CLUSTER_SERVICE_SNAPSHOT_TAKER_H

#include <Aeron.h>

#include "concurrent/YieldingIdleStrategy.h"
#include "aeron_cluster_codecs/ClusterTimeUnit.h"
#include "aeron_cluster_codecs/SnapshotMark.h"
#include "aeron_cluster_codecs/SnapshotMarker.h"


namespace aeron { namespace cluster { namespace service {

class SnapshotTaker
{
public:
  using SnapshotMark = codecs::SnapshotMark;
  constexpr static int ATTEMPTS = 3;
  explicit SnapshotTaker(std::shared_ptr<ExclusivePublication> publication);

  /**
   * Mark the beginning of the encoded snapshot.
   *
   * @param snapshotTypeId   type to identify snapshot within a cluster.
   * @param logPosition      at which the snapshot was taken.
   * @param leadershipTermId at which the snapshot was taken.
   * @param snapshotIndex    so the snapshot can be sectioned.
   * @param appVersion       associated with the snapshot from {@link ClusteredServiceContainer.Context#appVersion()}.
   */
  template<typename IdleStrategy = aeron::concurrent::YieldingIdleStrategy>
  inline bool markBegin(
    std::int64_t snapshotTypeId,
    std::int64_t logPosition,
    std::int64_t leadershipTermId,
    std::int32_t snapshotIndex,
    std::int32_t appVersion)
  {
    return markSnapshot<IdleStrategy>(
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
  template<typename IdleStrategy = aeron::concurrent::YieldingIdleStrategy>
  inline bool markEnd(
    std::int64_t snapshotTypeId,
    std::int64_t logPosition,
    std::int64_t leadershipTermId,
    std::int32_t snapshotIndex,
    std::int32_t appVersion)
  {
    return markSnapshot<IdleStrategy>(
      snapshotTypeId, logPosition, leadershipTermId, snapshotIndex, SnapshotMark::END, appVersion);
  }

  template<typename IdleStrategy = aeron::concurrent::YieldingIdleStrategy>
  bool markSnapshot(
    std::int64_t snapshotTypeId,
    std::int64_t logPosition,
    std::int64_t leadershipTermId,
    std::int32_t  snapshotIndex,
    SnapshotMark::Value snapshotMark,
    std::int32_t  appVersion);

protected:
  std::shared_ptr<ExclusivePublication> m_publication;
  void checkResult(std::int64_t result);
};

template<typename Idle>
inline bool SnapshotTaker::markSnapshot(
  std::int64_t snapshotTypeId,
  std::int64_t logPosition,
  std::int64_t leadershipTermId,
  std::int32_t  snapshotIndex,
  SnapshotMark::Value snapshotMark,
  std::int32_t  appVersion)
{ 
    using namespace codecs;

    Idle idle;

    BufferClaim bufferClaim;

    auto attempts = ATTEMPTS;

    while (--attempts >= 0)
    {
	std::int64_t result = m_publication->tryClaim(SnapshotMarker::sbeBlockAndHeaderLength(), bufferClaim);
	if (result > 0)
	{
	    auto buffer = bufferClaim.buffer();
	    SnapshotMarker marker;
	    marker
		.wrapAndApplyHeader(reinterpret_cast<char*>(buffer.buffer()), 0, bufferClaim.length())
		.typeId(snapshotTypeId)
		.logPosition(logPosition)
		.leadershipTermId(leadershipTermId)
		.index(snapshotIndex)
		.mark(snapshotMark)
		.timeUnit(ClusterTimeUnit::Value::NANOS)
		.appVersion(appVersion);
	
	    bufferClaim.commit();
	    return true;
	}
	checkResult(result);
	idle.idle();
    }
    return false;
}

}}}

#endif
