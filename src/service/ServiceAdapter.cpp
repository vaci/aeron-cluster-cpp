#include "ServiceAdapter.h"
#include "ClusteredServiceAgent.h"
#include "client/ClusterException.h"

#include "aeron_cluster_codecs/JoinLog.h"
#include "aeron_cluster_codecs/MessageHeader.h"
#include "aeron_cluster_codecs/RequestServiceAck.h"
#include "aeron_cluster_codecs/ServiceTerminationPosition.h"

namespace aeron { namespace cluster { namespace service {

using namespace codecs;

namespace {

static aeron::fragment_handler_t fragmentHandler(ServiceAdapter &adapter)
{
  return
    [&](AtomicBuffer &buffer, auto offset, auto length, Header &header)
    {
      adapter.onFragment(buffer, offset, length, header);
    };
}

}

ServiceAdapter::ServiceAdapter(
  std::shared_ptr<Subscription> subscription) :
  m_fragmentAssembler(fragmentHandler(*this)),
  m_fragmentHandler(m_fragmentAssembler.handler()),
  m_subscription(subscription)
{
}

ServiceAdapter::~ServiceAdapter()
{
}

void ServiceAdapter::onFragment(AtomicBuffer &buffer, util::index_t offset, util::index_t length, Header &header)
{
  std::cout << "ServiceAdapter::onFragment: offset=" << offset << " length=" << length << std::endl;
  MessageHeader messageHeader(buffer.sbeData() + offset, length);
  //messageHeader.wrap(buffer.sbeData(), offset, MessageHeader::sbeSchemaVersion(), length);

  std::cout << messageHeader << std::endl; 
  std::int32_t schemaId = messageHeader.schemaId();
  if (schemaId != MessageHeader::sbeSchemaId())
  {
    throw ClusterException(
      std::string("expected schemaId=") + std::to_string(MessageHeader::sbeSchemaId()) +
      ", actual=" + std::to_string(schemaId), SOURCEINFO);
  }

 
  switch (messageHeader.templateId())
  {
  case JoinLog::SBE_TEMPLATE_ID:
    {
      JoinLog joinLog;
      joinLog.wrapForDecode(
	buffer.sbeData(),
	offset + MessageHeader::encodedLength(),
	messageHeader.blockLength(),
	messageHeader.version(),
	1024);

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
    break;

  case ServiceTerminationPosition::SBE_TEMPLATE_ID:
    {
      std::cout << "ServiceTerminationPosition" << std::endl;
      ServiceTerminationPosition serviceTerminationPosition{};
      serviceTerminationPosition.wrapForDecode(
	buffer.sbeData(),
	offset + MessageHeader::encodedLength(),
	messageHeader.blockLength(),
	messageHeader.version(),
	1024);
      
      std::cout << serviceTerminationPosition << std::endl;
      m_agent->onServiceTerminationPosition(serviceTerminationPosition.logPosition());
    }
    break;

  case RequestServiceAck::SBE_TEMPLATE_ID:
    {
      std::cout << "RequestServiceAck" << std::endl;
      RequestServiceAck requestServiceAck{};
      requestServiceAck.wrapForDecode(
	buffer.sbeData(),
	offset + MessageHeader::encodedLength(),
	messageHeader.blockLength(),
	messageHeader.version(),
	1024);
      
      std::cout << requestServiceAck << std::endl;
      m_agent->onRequestServiceAck(requestServiceAck.logPosition());
    }
    break;
  }
}

}}}
