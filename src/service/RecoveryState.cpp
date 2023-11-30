#include "RecoveryState.h"
#include "client/ClusterException.h"

namespace aeron { namespace cluster { namespace service {

using ClusterException = client::ClusterException;

namespace RecoveryState {

std::int32_t findCounterId(CountersReader &counters, std::int32_t clusterId)
{
  auto buffer = counters.metaDataBuffer();

  for (int i = 0, size = counters.maxCounterId(); i < size; i++)
  {
    std::int32_t counterState = counters.getCounterState(i);
    if (counterState == CountersReader::RECORD_ALLOCATED &&
	counters.getCounterTypeId(i) == RECOVERY_STATE_TYPE_ID)
    {
      if (buffer.getInt32(CountersReader::metadataOffset(i) + CountersReader::KEY_OFFSET + CLUSTER_ID_OFFSET) == clusterId)
      {
	return i;
      }
    }
    else if (CountersReader::RECORD_UNUSED == counterState)
    {
      break;
    }
  }
  
  return CountersReader::NULL_COUNTER_ID;
}

std::int64_t getLogPosition(CountersReader &counters, std::int32_t counterId)
{
  auto buffer = counters.metaDataBuffer();

  if (counters.getCounterState(counterId) == CountersReader::RECORD_ALLOCATED &&
      counters.getCounterTypeId(counterId) == RECOVERY_STATE_TYPE_ID)
  {
    return buffer.getInt64(CountersReader::metadataOffset(counterId) + CountersReader::KEY_OFFSET + LOG_POSITION_OFFSET);
  }
  
  return NULL_VALUE;
}

std::int64_t getTimestamp(CountersReader &counters, std::int32_t counterId)
{
  auto buffer = counters.metaDataBuffer();

  if (counters.getCounterState(counterId) == CountersReader::RECORD_ALLOCATED &&
      counters.getCounterTypeId(counterId) == RECOVERY_STATE_TYPE_ID)
  {
    return buffer.getInt64(CountersReader::metadataOffset(counterId) + CountersReader::KEY_OFFSET + TIMESTAMP_OFFSET);
  }
  
  return NULL_VALUE;
}

std::int64_t getLeadershipTermId(CountersReader &counters, std::int32_t counterId)
{
  auto buffer = counters.metaDataBuffer();

  if (counters.getCounterState(counterId) == CountersReader::RECORD_ALLOCATED &&
      counters.getCounterTypeId(counterId) == RECOVERY_STATE_TYPE_ID)
  {
    return buffer.getInt64(CountersReader::metadataOffset(counterId) + CountersReader::KEY_OFFSET + LEADERSHIP_TERM_ID_OFFSET);
  }
  
  return NULL_VALUE;
}

std::int64_t getSnapshotRecordingId(CountersReader &counters, std::int32_t counterId, std::int32_t serviceId)
{
  auto buffer = counters.metaDataBuffer();

  if (counters.getCounterState(counterId) == CountersReader::RECORD_ALLOCATED &&
      counters.getCounterTypeId(counterId) == RECOVERY_STATE_TYPE_ID)
  {
    util::index_t recordOffset = CountersReader::metadataOffset(counterId);

    std::int32_t serviceCount = buffer.getInt32(recordOffset + CountersReader::KEY_OFFSET + SERVICE_COUNT_OFFSET);
    if (serviceId < 0 || serviceId >= serviceCount)
    {
      throw ClusterException(std::string("invalid serviceId ") + std::to_string(serviceId) + " for count of " + std::to_string(serviceCount), SOURCEINFO);
    }

    return buffer.getInt64(
      recordOffset + CountersReader::KEY_OFFSET + SNAPSHOT_RECORDING_IDS_OFFSET + (serviceId * sizeof(std::int64_t)));
  }

  throw ClusterException(std::string("active counter not found ") + std::to_string(counterId), SOURCEINFO);
}

}

}}} 
