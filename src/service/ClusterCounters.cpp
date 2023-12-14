#include "ClusterCounters.h"

namespace aeron { namespace cluster { namespace service {

namespace {

inline std::int32_t getCounterClusterId(CountersReader& counters, std::int32_t counterId)
{
  auto buffer = counters.metaDataBuffer();
  auto offset = CountersReader::metadataOffset(counterId) + CountersReader::KEY_OFFSET;
  return buffer.getInt32(offset);
}

}

namespace ClusterCounters {

std::int64_t allocate(
  std::shared_ptr<Aeron> aeron,
  const std::string &name,
  std::int32_t typeId,
  std::int32_t clusterId)
{
  auto label = name + " - clusterId=" + std::to_string(clusterId);
  return aeron->addCounter(typeId, reinterpret_cast<std::uint8_t*>(&clusterId), sizeof(clusterId), label);
}

std::shared_ptr<Counter> find(CountersReader& counters, std::int32_t typeId, std::int32_t clusterId)
{
  for (int i = 0; i < counters.maxCounterId(); i++)
  {
    std::int32_t counterState = counters.getCounterState(i);

    if (counterState == CountersReader::RECORD_ALLOCATED)
    {
      if (typeId == counters.getCounterTypeId(i) && clusterId == getCounterClusterId(counters, i))
      {
	return std::make_shared<Counter>(counters, counters.getCounterRegistrationId(i), i);
      }
    }
    else if (counterState == CountersReader::RECORD_UNUSED)
    {
      break;
    }
  }

  return nullptr;
}

}

}}}
