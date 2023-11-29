#include "ClusteredServiceAgent.h"

#include "client/ClusterException.h"

namespace aeron { namespace cluster { namespace service {

using ClusterException = client::ClusterException;

ClusteredServiceAgent::AsyncConnect::AsyncConnect(
  ClusteredServiceContext & ctx,
  std::int64_t publicationId,
  std::int64_t subscriptionId) :
  m_ctx(ctx),
  m_aeron(ctx.aeron()),
  m_publicationId(publicationId),
  m_subscriptionId(subscriptionId)
{
}

std::shared_ptr<ClusteredServiceAgent> ClusteredServiceAgent::AsyncConnect::poll()
{
  if (!m_publication)
  {
    m_publication = m_aeron->findExclusivePublication(m_publicationId);
  }
  if (!m_subscription)
  {
    m_subscription = m_aeron->findSubscription(m_subscriptionId);
  }
  if (m_publication && !m_proxy)
  {
    m_proxy = std::make_shared<ConsensusModuleProxy>(m_publication);
  }
  if (m_subscription && !m_serviceAdapter)
  {
    m_serviceAdapter = std::make_shared<ServiceAdapter>(m_subscription);
  }
  if (0 == m_step && m_proxy && m_serviceAdapter)
  {
    if (!m_proxy->publication()->isConnected())
    {
      return {};
    }

    m_step = 1;
  }

  if (m_step == 1)
  {
    auto agent = std::make_shared<ClusteredServiceAgent>(m_ctx, m_proxy, m_serviceAdapter);
    m_serviceAdapter->agent(agent);
    return agent;
  }

  return {};
}

std::shared_ptr<ClusteredServiceAgent::AsyncConnect> ClusteredServiceAgent::asyncConnect(ClusteredServiceContext &ctx)
{
  ctx.conclude();

  std::shared_ptr<Aeron> aeron = ctx.aeron();
  
  const std::int64_t publicationId = aeron->addExclusivePublication(
    ctx.controlChannel(), ctx.consensusModuleStreamId());

  const std::int64_t subscriptionId = aeron->addSubscription(
    ctx.controlChannel(), ctx.serviceStreamId());
  
  return std::make_shared<AsyncConnect>(ctx, publicationId, subscriptionId);

}

ClusteredServiceAgent::ClusteredServiceAgent(
  ClusteredServiceContext &ctx,
  std::shared_ptr<ConsensusModuleProxy> proxy,
  std::shared_ptr<ServiceAdapter> serviceAdapter) :
  m_ctx(ctx),
  m_aeron(ctx.aeron()),
  m_service(ctx.clusteredService()),
  m_serviceId(ctx.serviceId()),
  m_subscriptionAlias(std::string("log-sc-") + std::to_string(ctx.serviceId())),
  m_consensusModuleProxy(proxy),
  m_serviceAdapter(serviceAdapter)
{
}

std::shared_ptr<ClientSession> ClusteredServiceAgent::getClientSession(std::int64_t clusterSessionId)
{
  return nullptr;
}

bool ClusteredServiceAgent::closeClientSession(std::int64_t clusterSessionId)
{
  checkForValidInvocation();

  auto clientSession = m_sessionByIdMap.find(clusterSessionId);
  if (clientSession == m_sessionByIdMap.end())
  {
    throw ClusterException(
      std::string("unknown clusterSessionId: ") + std::to_string(clusterSessionId), SOURCEINFO);
  }

  if (clientSession->second->isClosing())
  {
    return true;
  }

  if (m_consensusModuleProxy->closeSession(clusterSessionId))
  {
    clientSession->second->markClosing();
    return true;
  }
  
  return false;
}

std::int64_t ClusteredServiceAgent::offer(AtomicBuffer& buffer)
{
  checkForValidInvocation();
  m_header.clusterSessionId(context().serviceId());

  AtomicBuffer headerBuffer(reinterpret_cast<std::uint8_t*>(m_header.buffer()), m_header.bufferLength());
  return m_consensusModuleProxy->offer(headerBuffer, buffer);
}


void ClusteredServiceAgent::onTimerEvent(
  std::int64_t logPosition, std::int64_t correlationId, std::int64_t timestamp)
{
  m_logPosition = logPosition;
  m_clusterTime = timestamp;
  m_service->onTimerEvent(correlationId, timestamp);
}

void ClusteredServiceAgent::checkForValidInvocation()
{
  if (nullptr != m_activeLifecycleCallbackName)
  {
    throw ClusterException(
      std::string("sending messages or scheduling timers is not allowed from ") + m_activeLifecycleCallbackName, SOURCEINFO);
  }

  if (m_isBackgroundInvocation)
  {
    throw ClusterException(
      std::string("sending messages or scheduling timers is not allowed from ClusteredService.doBackgroundWork"), SOURCEINFO);
  }
}

}}}
