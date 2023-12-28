#include "ServiceAdapter.h"
#include "ClusteredServiceAgent.h"

#include "aeron_cluster_codecs/JoinLog.h"
#include "aeron_cluster_codecs/MessageHeader.h"
#include "aeron_cluster_codecs/RequestServiceAck.h"
#include "aeron_cluster_codecs/ServiceTerminationPosition.h"

using namespace aeron;
using namespace aeron::cluster::service;
using namespace aeron::cluster::codecs;

static fragment_handler_t fragmentHandler(ServiceAdapter &adapter)
{
  return
    [&](AtomicBuffer &buffer, auto offset, auto length, Header &header)
    {
      adapter.onFragment(buffer, offset, length, header);
    };
}

ServiceAdapter::ServiceAdapter(
  std::shared_ptr<Subscription> subscription,
  int fragmentLimit) :
  m_fragmentAssembler(fragmentHandler(*this)),
  m_fragmentHandler(m_fragmentAssembler.handler()),
  m_subscription(std::move(subscription)),
  m_fragmentLimit(fragmentLimit)
{
}

void ServiceAdapter::onFragment(AtomicBuffer &buffer, util::index_t offset, util::index_t length, Header &header)
{
  MessageHeader msgHeader(
    buffer.sbeData() + offset,
    static_cast<std::uint64_t>(length),
    MessageHeader::sbeSchemaVersion());
  
  const std::uint16_t schemaId = msgHeader.schemaId();
  if (schemaId != MessageHeader::sbeSchemaId())
  {
    throw ClusterException(
      std::string("expected schemaId=") + std::to_string(MessageHeader::sbeSchemaId()) +
      ", actual=" + std::to_string(schemaId), SOURCEINFO);
  }

  if (JoinLog::sbeTemplateId() == msgHeader.templateId())
  {
    JoinLog joinLog(
      buffer.sbeData() + offset + MessageHeader::encodedLength(),
      static_cast<std::uint64_t>(length) - MessageHeader::encodedLength(),
      msgHeader.blockLength(),
      msgHeader.version());
    
    std::cout << joinLog << std::endl;
    m_agent->onJoinLog(
      joinLog.logPosition(),
      joinLog.maxLogPosition(),
      joinLog.memberId(),
      joinLog.logSessionId(),
      joinLog.logStreamId(),
      joinLog.isStartup() == BooleanType::TRUE,
      Cluster::getRole(joinLog.role()),
      joinLog.logChannel());
  }
  else if (ServiceTerminationPosition::sbeTemplateId() == msgHeader.templateId())
  {
    std::cout << "ServiceTerminationPosition" << std::endl;
    ServiceTerminationPosition serviceTerminationPosition(
      buffer.sbeData() + offset + MessageHeader::encodedLength(),
      static_cast<std::uint64_t>(length) - MessageHeader::encodedLength(),
      msgHeader.blockLength(),
      msgHeader.version());
    
    std::cout << serviceTerminationPosition << std::endl;
    m_agent->onServiceTerminationPosition(serviceTerminationPosition.logPosition());
  }
  else if (RequestServiceAck::sbeTemplateId() == msgHeader.templateId())
  {
    std::cout << "RequestServiceAck" << std::endl;
    RequestServiceAck requestServiceAck(
      buffer.sbeData() + offset + MessageHeader::encodedLength(),
      static_cast<std::uint64_t>(length) - MessageHeader::encodedLength(),
      msgHeader.blockLength(),
      msgHeader.version());
    
    std::cout << requestServiceAck << std::endl;
      m_agent->onRequestServiceAck(requestServiceAck.logPosition());
  }
}

