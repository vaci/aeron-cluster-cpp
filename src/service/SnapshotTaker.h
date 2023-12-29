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
    inline void markBegin(
	std::int64_t snapshotTypeId,
	std::int64_t logPosition,
	std::int64_t leadershipTermId,
	std::int32_t snapshotIndex,
	std::int32_t appVersion)
    {
	markSnapshot<IdleStrategy>(
	    snapshotTypeId, logPosition, leadershipTermId, snapshotIndex,
	    SnapshotMark::BEGIN, appVersion);
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
    template <typename IdleStrategy = aeron::concurrent::YieldingIdleStrategy>
    inline void markEnd(
	std::int64_t snapshotTypeId,
	std::int64_t logPosition,
	std::int64_t leadershipTermId,
	std::int32_t snapshotIndex,
	std::int32_t appVersion)
    {
	markSnapshot<IdleStrategy>(
	    snapshotTypeId, logPosition, leadershipTermId, snapshotIndex,
	    SnapshotMark::END, appVersion);
    }

    template <typename IdleStrategy = aeron::concurrent::YieldingIdleStrategy>
    void markSnapshot(
	std::int64_t snapshotTypeId,
	std::int64_t logPosition,
	std::int64_t leadershipTermId,
	std::int32_t  snapshotIndex,
	SnapshotMark::Value snapshotMark,
	std::int32_t  appVersion);

protected:
    std::shared_ptr<ExclusivePublication> m_publication;

    template<typename IdleStrategy>
    void offer(AtomicBuffer &buffer, util::index_t offset, util::index_t length);

    void checkResult(std::int64_t result);
};

template <typename IdleStrategy>
inline void SnapshotTaker::markSnapshot(
    std::int64_t snapshotTypeId,
    std::int64_t logPosition,
    std::int64_t leadershipTermId,
    std::int32_t  snapshotIndex,
    SnapshotMark::Value snapshotMark,
    std::int32_t  appVersion)
{ 
    using namespace codecs;
    constexpr auto length = SnapshotMarker::sbeBlockAndHeaderLength();

    std::cout << "Snap mark length: " << length << std::endl;
    char data[length];
    SnapshotMarker marker;
    marker
	.wrapAndApplyHeader(data, 0, length)
	.typeId(snapshotTypeId)
	.logPosition(logPosition)
	.leadershipTermId(leadershipTermId)
	.index(snapshotIndex)
	.mark(snapshotMark)
	.timeUnit(ClusterTimeUnit::Value::NANOS)
	.appVersion(appVersion);

    AtomicBuffer buffer(reinterpret_cast<unsigned char*>(data), length);
    offer<IdleStrategy>(buffer, 0, length);

    /*
    IdleStrategy idle;

    while (true)
    {
	BufferClaim bufferClaim;
	std::int64_t result = m_publication->tryClaim(length, bufferClaim);
	if (result > 0)
	{
	    auto buffer = bufferClaim.buffer();
	    auto offset = bufferClaim.offset();

	    SnapshotMarker marker;
	    marker
		.wrapAndApplyHeader(buffer.sbeData() + offset, 0, length)
		.typeId(snapshotTypeId)
		.logPosition(logPosition)
		.leadershipTermId(leadershipTermId)
		.index(snapshotIndex)
		.mark(snapshotMark)
		.timeUnit(ClusterTimeUnit::Value::NANOS)
		.appVersion(appVersion);
	
	    bufferClaim.commit();
	    return;
	}
	checkResult(result);
	idle.idle();
    }
    */
}

template<typename IdleStrategy>
void SnapshotTaker::offer(AtomicBuffer &buffer, util::index_t offset, util::index_t length)
{
    IdleStrategy idle;
    while (true)
    {
	const std::int64_t result = m_publication->offer(buffer, offset, length);
	if (result > 0)
	{
	    return;
	}
	checkResult(result);
	
	idle.idle();
    }
}


}}}

#endif
