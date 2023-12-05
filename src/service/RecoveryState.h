#ifndef AERON_CLUSTER_SERVICE_RECOVERY_STATE_H
#define AERON_CLUSTER_SERVICE_RECOVERY_STATE_H

#include <Aeron.h>

#include "ClusteredServiceConfiguration.h"

namespace aeron { namespace cluster { namespace service {

namespace RecoveryState
{
/**
 * Type id of a recovery state counter.
 */
constexpr std::int32_t RECOVERY_STATE_TYPE_ID = AERON_COUNTER_CLUSTER_RECOVERY_STATE_TYPE_ID;

/**
 * Human-readable name for the counter.
 */
constexpr const char* NAME = "Cluster recovery: leadershipTermId=";

constexpr std::int32_t LEADERSHIP_TERM_ID_OFFSET = 0;
constexpr std::int32_t LOG_POSITION_OFFSET = LEADERSHIP_TERM_ID_OFFSET + sizeof(std::int64_t);
constexpr std::int32_t TIMESTAMP_OFFSET = LOG_POSITION_OFFSET + sizeof(std::int64_t);
constexpr std::int32_t CLUSTER_ID_OFFSET = TIMESTAMP_OFFSET + sizeof(std::int64_t);
constexpr std::int32_t SERVICE_COUNT_OFFSET = CLUSTER_ID_OFFSET + sizeof(std::int32_t);
constexpr std::int32_t SNAPSHOT_RECORDING_IDS_OFFSET = SERVICE_COUNT_OFFSET + sizeof(std::int32_t);

/**
 * Find the active counter id for recovery state.
 *
 * @param counters  to search within.
 * @param clusterId to constrain the search.
 * @return the counter id if found otherwise {@link CountersReader#NULL_COUNTER_ID}.
 */
std::shared_ptr<Counter> findCounter(CountersReader &counters, std::int32_t clusterId);

/**
 * Get the position at which the snapshot was taken. {@link Aeron#NULL_VALUE} if no snapshot for recovery.
 *
 * @param counters  to search within.
 * @param counterId for the active recovery counter.
 * @return the log position if found otherwise {@link Aeron#NULL_VALUE}.
 */
std::int64_t getLogPosition(CountersReader &counters, std::int32_t counterId);

/**
 * Get the timestamp at the beginning of recovery. {@link Aeron#NULL_VALUE} if no snapshot for recovery.
 *
 * @param counters  to search within.
 * @param counterId for the active recovery counter.
 * @return the timestamp if found otherwise {@link Aeron#NULL_VALUE}.
 */
std::int64_t getTimestamp(CountersReader &counters, std::int32_t counterId);

/**
 * Get the leadership term id for the snapshot state. {@link Aeron#NULL_VALUE} if no snapshot for recovery.
 *
 * @param counters  to search within.
 * @param counterId for the active recovery counter.
 * @return the leadership term id if found otherwise {@link Aeron#NULL_VALUE}.
 */
std::int64_t getLeadershipTermId(CountersReader &counters, std::int32_t counterId);

/**
 * Get the recording id of the snapshot for a service.
 *
 * @param counters  to search within.
 * @param counterId for the active recovery counter.
 * @param serviceId for the snapshot required.
 * @return the count of replay terms if found otherwise {@link Aeron#NULL_VALUE}.
 */
std::int64_t getSnapshotRecordingId(CountersReader &counters, std::int32_t counterId, std::int32_t serviceId);

};

}}}

#endif
