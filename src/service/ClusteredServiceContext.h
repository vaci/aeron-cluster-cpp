#ifndef AERON_CLUSTER_SERVICE_CLUSTERED_SERVICE_CONTEXT_H
#define AERON_CLUSTER_SERVICE_CLUSTERED_SERVICE_CONTEXT_H

#include "ClusteredService.h"

namespace aeron { namespace cluster { namespace service {

class ClusteredServiceContext
{
public:
  using this_t = ClusteredServiceContext;
  ClusteredServiceContext() {}

  std::shared_ptr<Aeron> aeron()
  {
    return m_aeron;
  }

  this_t &serviceId(std::shared_ptr<Aeron> aeron)
  {
    m_aeron = aeron;
    return *this;
  }

  std::int32_t serviceId() const
  {
    return m_serviceId;
  }

  this_t &serviceId(std::int32_t serviceId)
  {
    m_serviceId = serviceId;
    return *this;
  }

  const std::string &controlChannel() const
  {
    return m_controlChannel;
  }

  this_t &controlChannel(const std::string &controlChannel)
  {
    m_controlChannel = controlChannel;
    return *this;
  }

  std::int32_t consensusModuleStreamId() const
  {
    return m_consensusModuleStreamId;
  }

  this_t &consensusModuleStreamId(std::int32_t streamId)
  {
    m_consensusModuleStreamId = streamId;
    return *this;
  }

  std::int32_t serviceStreamId() const
  {
    return m_serviceStreamId;
  }

  this_t &serviceStreamId(std::int32_t streamId)
  {
    m_serviceStreamId = streamId;
    return *this;
  }

  std::shared_ptr<ClusteredService> clusteredService()
  {
    return m_clusteredService;
  }

  this_t &clusteredService(std::shared_ptr<ClusteredService> clusteredService)
  {
    m_clusteredService = clusteredService;
    return *this;
  }

  void conclude();
private:
  std::shared_ptr<Aeron> m_aeron;
  std::int32_t m_serviceId;
  std::string m_controlChannel;
  std::int32_t m_consensusModuleStreamId;
  std::int32_t m_serviceStreamId;
  std::shared_ptr<ClusteredService> m_clusteredService;
};

}}}
#endif
