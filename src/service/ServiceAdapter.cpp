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
    [&](AtomicBuffer &buffer, util::index_t offset, util::index_t length, Header &header)
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
  std::cout << "ServiceAdapter::onFragment" << std::endl;
  MessageHeader messageHeader;

  std::int32_t schemaId = messageHeader.schemaId();
  if (schemaId != MessageHeader::sbeSchemaId())
  {
    throw ClusterException(
      std::string("expected schemaId=") + std::to_string(MessageHeader::sbeSchemaId()) +
      ", actual=" + std::to_string(schemaId), SOURCEINFO);
  }
  
  switch (messageHeader.templateId())
  {
  case JoinLog::sbeTemplateId():
    {
      JoinLog joinLog;
      joinLog.wrapForDecode(
	reinterpret_cast<char*>(buffer.buffer()),
	offset + MessageHeader::encodedLength(),
	messageHeader.blockLength(),
	messageHeader.version(),
	length - MessageHeader::encodedLength());

      m_agent->onJoinLog(
	joinLog.logPosition(),
	joinLog.maxLogPosition(),
	joinLog.memberId(),
	joinLog.logSessionId(),
	joinLog.logStreamId(),
	joinLog.isStartup() == BooleanType::TRUE,
	Cluster::getRole(joinLog.role()),
	joinLog.logChannel());
      break;
    }
  case ServiceTerminationPosition::sbeTemplateId():
    {
      ServiceTerminationPosition serviceTerminationPosition;
      serviceTerminationPosition.wrapForDecode(
	reinterpret_cast<char*>(buffer.buffer()),
	offset + MessageHeader::encodedLength(),
	messageHeader.blockLength(),
	messageHeader.version(),
	length - MessageHeader::encodedLength());
      
      m_agent->onServiceTerminationPosition(serviceTerminationPosition.logPosition());
      break;
    }
    
  case RequestServiceAck::sbeTemplateId():
    {
      RequestServiceAck requestServiceAck;
      requestServiceAck.wrapForDecode(
	reinterpret_cast<char*>(buffer.buffer()),
	offset + MessageHeader::encodedLength(),
	messageHeader.blockLength(),
	messageHeader.version(),
	length - MessageHeader::encodedLength());
      
      m_agent->onRequestServiceAck(requestServiceAck.logPosition());
      break;
    }
  }
}

}}}
