#include "AeronCluster.h"

#include "ChannelUri.h"
#include "ClusterException.h"

#include "aeron_cluster_codecs/SessionConnectRequest.h"
#include "aeron_cluster_codecs/ChallengeResponse.h"

namespace aeron { namespace cluster { namespace client {

using namespace codecs;

constexpr std::int32_t FRAGMENT_LIMIT = 10;

const char* AeronCluster::AsyncConnect::stepName(int step)
{
  switch (step)
  {
  case -1: return "CREATE_EGRESS_SUBSCRIPTION";
  case 0: return "CREATE_INGRESS_PUBLICATIONS";
  case 1: return "AWAIT_PUBLICATION_CONNECTED";
  case 2: return "SEND_MESSAGE";
  case 3: return "POLL_RESPONSE";
  case 4: return "CONCLUDE_CONNECT";
  case 5: return "DONE";
  default: return "<unknown>";
  }
}

AeronCluster::MemberIngressMap parseIngressEndpoints(const std::string & endpoints)
{
  AeronCluster::MemberIngressMap endpointByIdMap;

  for (auto pos = endpoints.find(',');
       pos != std::string::npos;
       pos = endpoints.find(',', pos))
  { 
    std::string endpoint = endpoints.substr(0, pos);
    auto sep = endpoint.find('=');
    if (std::string::npos == sep)
    {
      throw ClusterException("endpoint missing '=' separator: " + endpoints, SOURCEINFO);
    }

    std::int32_t memberId = std::stoi(endpoint.substr(0, sep));
    endpointByIdMap.insert({memberId, {memberId, endpoint.substr(sep + 1)}});
  }

  return endpointByIdMap;

}

void AeronCluster::Context::conclude()
{
  if (!m_aeron)
  {
    aeron::Context ctx;

    ctx.aeronDir(m_aeronDirectoryName);
    m_aeron = Aeron::connect(ctx);
    m_ownsAeronClient = true;
  }

  if (nullptr == m_onSessionMessage)
  {
    m_onSessionMessage = defaultSessionMessageConsumer();
  }
  if (nullptr == m_onSessionEvent)
  {
    m_onSessionEvent = defaultSessionEventConsumer();
  }
  if (nullptr == m_onNewLeaderEvent)
  {
    m_onNewLeaderEvent = defaultNewLeaderEventConsumer();
  }
  if (nullptr == m_onAdminResponse)
  {
    m_onAdminResponse = defaultAdminResponseConsumer();
  }
}

void AeronCluster::AsyncConnect::createEgressSubscription()
{
  if (NULL_VALUE == m_egressRegistrationId)
  {
    m_egressRegistrationId = m_ctx->aeron()->addSubscription(
      m_ctx->egressChannel(), m_ctx->egressStreamId());
  }

  m_egressSubscription = m_ctx->aeron()->findSubscription(m_egressRegistrationId);
  if (nullptr != m_egressSubscription)
  {
    m_egressPoller = std::make_unique<EgressPoller>(m_egressSubscription, FRAGMENT_LIMIT);
    step(0);
  }
}

void AeronCluster::AsyncConnect::createIngressPublications()
{
  if ("" == m_ctx->ingressEndpoints())
  {
    if (NULL_VALUE == m_ingressRegistrationId)
    {
      m_ingressRegistrationId = m_ctx->aeron()->addPublication(m_ctx->ingressChannel(), m_ctx->ingressStreamId());
    }

    if (nullptr == m_ingressPublication)
    {
      m_ingressPublication = m_ctx->aeron()->findExclusivePublication(m_ingressRegistrationId);
    }

    if (nullptr != m_ingressPublication)
    {
      m_ingressRegistrationId = NULL_VALUE;
      step(1);
    }
  }
  else
  {
    int publicationCount = 0;
    int failureCount = 0;
    auto channelUri = ChannelUri::parse(m_ctx->ingressChannel());

    for (auto&& entry: m_memberByIdMap)
    {
      auto& member = entry.second;
      try
      {
	if (channelUri->media() == UDP_MEDIA)
	{
	  channelUri->put(ENDPOINT_PARAM_NAME, member.m_endpoint);
	}

	//if (nullptr != member.m_publicationException)
	//{
	//  failureCount++;
	//  continue;
	//}
	  
	if (NULL_VALUE == member.m_registrationId)
	{
	  member.m_registrationId = m_ctx->aeron()->addPublication(channelUri->toString(), m_ctx->ingressStreamId());
	}

	if (nullptr == member.m_publication)
	{
	  member.m_publication = m_ctx->aeron()->findExclusivePublication(member.m_registrationId);
	}

	if (nullptr != member.m_publication)
	{
	  publicationCount++;
	}
      }
      catch (const RegistrationException &ex)
      {
	//member.m_publicationException = std::make_unique<RegistrationException>(ex);
      }
    }
    
    if (publicationCount + failureCount == m_memberByIdMap.size())
    {
      if (0 == publicationCount)
      {
	//throw *(m_memberByIdMap.begin()->second.m_publicationException);
      }
	
      step(1);
    }
  }
}

void AeronCluster::AsyncConnect::awaitPublicationConnected()
{
  auto responseChannel = m_egressSubscription->tryResolveChannelEndpointPort();
  if ("" != responseChannel)
  {
    if (nullptr == m_ingressPublication)
    {
      for (auto&& entry: m_memberByIdMap)
      {
	auto& member = entry.second;
	if (nullptr != member.m_publication && member.m_publication->isConnected())
	{
	  m_ingressPublication = member.m_publication;
	  prepareConnectRequest(responseChannel);
	  break;
	}
      }
    }
    else if (m_ingressPublication->isConnected())
    {
      prepareConnectRequest(responseChannel);
    }
  }
}

void AeronCluster::AsyncConnect::prepareConnectRequest(const std::string &responseChannel)
{
  m_correlationId = m_ctx->aeron()->nextCorrelationId();
  auto encodedCredentials = m_ctx->credentialsSupplier().m_encodedCredentials();

  SessionConnectRequest request(
    (char*)m_buffer, 0, 1024, SessionConnectRequest::sbeBlockLength(), 0);
  
  request
    .correlationId(m_correlationId)
    .responseStreamId(m_ctx->egressStreamId())
    .version(0)
    .putResponseChannel(responseChannel.c_str(), responseChannel.size())
    .putEncodedCredentials(encodedCredentials.first, encodedCredentials.second);
    
 
  m_messageLength = MessageHeader::encodedLength() + request.encodedLength();
  step(2);
}

void AeronCluster::AsyncConnect::sendMessage()
{
  AtomicBuffer buffer(m_buffer, m_messageLength);
  std::int64_t result = m_ingressPublication->offer(buffer);
  if (result > 0)
  {
    step(3);
  }
  else if (PUBLICATION_CLOSED == result || NOT_CONNECTED == result)
  {
    throw ClusterException("unexpected loss of connection to cluster", SOURCEINFO);
  }
}

void AeronCluster::AsyncConnect::pollResponse()
{
  if (m_egressPoller->poll() > 0 &&
      m_egressPoller->isPollComplete() &&
      m_egressPoller->correlationId() == m_correlationId)
  {
    if (m_egressPoller->isChallenged())
    {
      m_correlationId = NULL_VALUE;
      m_clusterSessionId = m_egressPoller->clusterSessionId();
      auto encodedChallenge = m_egressPoller->encodedChallenge();
      prepareChallengeResponse(m_ctx->credentialsSupplier().m_onChallenge(encodedChallenge));
      return;
    }

    switch (m_egressPoller->eventCode())
    {
    case EventCode::OK:
      m_leadershipTermId = m_egressPoller->leadershipTermId();
      m_leaderMemberId = m_egressPoller->leaderMemberId();
      m_clusterSessionId = m_egressPoller->clusterSessionId();
      m_egressImage = std::make_unique<Image>(m_egressPoller->egressImage());
      step(4);
      break;

    case EventCode::ERROR:
      throw ClusterException(m_egressPoller->detail(), SOURCEINFO);

    case EventCode::REDIRECT:
      updateMembers();
      break;

    case EventCode::AUTHENTICATION_REJECTED:
      throw ClusterException(m_egressPoller->detail(), SOURCEINFO);

    case EventCode::CLOSED:
    case EventCode::NULL_VALUE:
      break;
    }
  }
}


void AeronCluster::AsyncConnect::prepareChallengeResponse(std::pair<const char*, uint32_t> encodedCredentials)
{
  m_correlationId = m_ctx->aeron()->nextCorrelationId();

  ChallengeResponse response(
    (char*)m_buffer, 0, 1024, ChallengeResponse::sbeBlockLength(), 0);
  
  response
    .correlationId(m_correlationId)
    .clusterSessionId(m_clusterSessionId)
    .putEncodedCredentials(encodedCredentials.first, encodedCredentials.second);

  m_messageLength = MessageHeader::encodedLength() + response.encodedLength();

  step(2);
}

void AeronCluster::AsyncConnect::updateMembers()
{
  m_leaderMemberId = m_egressPoller->leaderMemberId();

  // TODO avoid closing the existing publication?
  m_ingressRegistrationId = NULL_VALUE;
  if (m_ingressPublication != nullptr)
  {
    m_ingressPublication->close();
    m_ingressPublication = nullptr;
  }
  
  m_memberByIdMap = parseIngressEndpoints(m_egressPoller->detail());

  step(1);
}

std::shared_ptr<AeronCluster> AeronCluster::AsyncConnect::concludeConnect()
{

  if (m_ingressPublication == nullptr)
  {
    m_ingressPublication = m_ctx->aeron()->findExclusivePublication(m_ingressRegistrationId);
    return nullptr;
  }

  auto aeronCluster = std::make_shared<AeronCluster>(
    std::move(m_ctx),
    std::move(m_ingressPublication),
    std::move(m_egressSubscription),
    *m_egressImage,
    m_memberByIdMap,
    m_clusterSessionId,
    m_leadershipTermId,
    m_leaderMemberId
  );
  m_memberByIdMap.erase(m_leaderMemberId);
  step(5);
  return aeronCluster;
}

std::shared_ptr<AeronCluster> AeronCluster::AsyncConnect::poll()
{
  std::shared_ptr<AeronCluster> aeronCluster = nullptr;

  checkDeadline();

  std::cout << stepName(m_step) << std::endl;

  switch (m_step)
  {
  case -1:
    createEgressSubscription();
    break;
  case 0:
    createIngressPublications();
    break;
  case 1:
    awaitPublicationConnected();
    break;
  case 2:
    sendMessage();
    break;
  case 3:
    pollResponse();
    break;
  case 4:
    aeronCluster = concludeConnect();
    break;
  }

  return aeronCluster;
}

void AeronCluster::AsyncConnect::checkDeadline()
{
  if (m_deadlineNs < 0)
  {
    bool isConnected = nullptr != m_egressSubscription && m_egressSubscription->isConnected();
    
    std::string endpointPort = nullptr != m_egressSubscription
      ? m_egressSubscription->tryResolveChannelEndpointPort()
      : "<unknown>";

    TimeoutException ex(
      std::string("cluster connect timeout: step=") + stepName(m_step) +
      " messageTimeout=" + std::to_string(m_ctx->messageTimeoutNs()) + "ns" +
      " ingressChannel=" + m_ctx->ingressChannel() +
      " ingressEndpoints=" + m_ctx->ingressEndpoints() +
      " ingressPublication=" + m_ingressPublication->channel() +
      " egress.isConnected=" + (isConnected ? "true" : "false") +
      " responseChannel=" + endpointPort,
      SOURCEINFO);

    for (auto &&entry: m_memberByIdMap)
    {
      auto &member = entry.second;
      // TODO
      /*
      if (nullptr != member.m_publicationException)
      {
      }
      */
    }

    throw ex;
  }
}
  
void AeronCluster::AsyncConnect::close()
{
  if (5 != m_step)
  {
    m_ingressPublication->close();
    m_egressSubscription->closeAndRemoveImages();
    for (auto&& entry: m_memberByIdMap)
    {
	entry.second.close();
    }
    m_ctx->close();
  }  
}

std::shared_ptr<AeronCluster> AeronCluster::connect(AeronCluster::Context &context)
{
  // TODO
  return nullptr;
}

AeronCluster::AeronCluster(
  std::unique_ptr<Context> ctx,
  std::shared_ptr<ExclusivePublication> publication,
  std::shared_ptr<Subscription> subscription,
  Image egressImage,
  MemberIngressMap memberByIdMap,
  std::int64_t clusterSessionId,
  std::int64_t leadershipTermId,
  std::int32_t leaderMemberId):
  m_ctx(std::move(ctx)),
  m_publication(std::move(publication)),
  m_subscription(std::move(subscription)),
  m_egressImage(std::move(egressImage)),
  m_memberByIdMap(std::move(memberByIdMap)),
  m_clusterSessionId(clusterSessionId),
  m_leadershipTermId(leadershipTermId)
{
}

AeronCluster::AsyncConnect::AsyncConnect(AeronCluster::Context &ctx, std::int64_t deadlineNs) :
  m_deadlineNs(deadlineNs),
  m_ctx(std::make_unique<Context>(ctx))
{
}

AeronCluster::AsyncConnect AeronCluster::asyncConnect(AeronCluster::Context &ctx)
{

  try
  {
    ctx.conclude();
    const long long deadlineNs = systemNanoClock() + ctx.messageTimeoutNs();
    return AsyncConnect{ctx, deadlineNs};
  }
  catch (...)
  {
    ctx.close();
    throw;
  }
}

}}}
