#include "ClusterCounters.h"

namespace aeron { namespace cluster { namespace service {

std::int64_t ClusterCounters::allocate(
  std::shared_ptr<Aeron> aeron,
  const std::string &name,
  std::int32_t typeId,
  std::int32_t clusterId)
{
  std::string label = name + " - clusterId=" + std::to_string(clusterId);
  return aeron->addCounter(
    typeId, reinterpret_cast<std::uint8_t*>(&clusterId), sizeof(clusterId), label);
}


std::int32_t ClusterCounters::find(CountersReader& counters, std::int32_t typeId, std::int32_t clusterId)
{
  auto buffer = counters.metaDataBuffer();

  for (int i = 0; i < counters.maxCounterId(); i++)
  {
    std::int32_t counterState = counters.getCounterState(i);

    if (counterState == CountersReader::RECORD_ALLOCATED)
    {
      if (counters.getCounterTypeId(i) == typeId &&
	  buffer.getInt32(CountersReader::metadataOffset(i) + CountersReader::KEY_OFFSET) == clusterId)
      {
	return i;
      }
    }
    else if (CountersReader::RECORD_UNUSED == counterState)
    {
      break;
    }
  }

  return NULL_VALUE;
}

}}}
