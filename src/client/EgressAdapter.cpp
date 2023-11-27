#include "EgressAdapter.h"
#include "ClusterException.h"
#include "aeron_cluster_client/MessageHeader.h"
#include "aeron_cluster_client/SessionMessageHeader.h"
#include "aeron_cluster_client/SessionEvent.h"
#include "aeron_cluster_client/NewLeaderEvent.h"
#include "aeron_cluster_client/AdminResponse.h"

namespace aeron { namespace cluster { namespace client {

namespace {

static aeron::fragment_handler_t fragmentHandler(EgressAdapter &adapter)
{
  return
    [&](AtomicBuffer &buffer, util::index_t offset, util::index_t length, Header &header)
    {
      adapter.onFragment(buffer, offset, length, header);
    };
}

}

EgressAdapter::EgressAdapter(
  const on_session_message_t &onSessionMessage,
  const on_session_event_t &onSessionEvent,
  const on_new_leader_event_t &onNewLeaderEvent,
  const on_admin_response_t &onAdminResponse,
  int64_t clusterSessionId,
  std::shared_ptr<Subscription> subscription,
  int fragmentLimit) : 
  m_fragmentAssembler(fragmentHandler(*this)),
  m_fragmentHandler(m_fragmentAssembler.handler()),
  m_clusterSessionId(clusterSessionId),
  m_subscription(subscription),
  m_onSessionMessage(onSessionMessage),
  m_onSessionEvent(onSessionEvent),
  m_onNewLeaderEvent(onNewLeaderEvent),
  m_onAdminResponse(onAdminResponse),
  m_fragmentLimit(fragmentLimit)
{
}

void EgressAdapter::onFragment(AtomicBuffer buffer, util::index_t offset, util::index_t length, Header& header)
{
  MessageHeader msgHeader(
    buffer.sbeData() + offset,
    static_cast<std::uint64_t>(length),
    MessageHeader::sbeSchemaVersion());

  const std::uint16_t schemaId = msgHeader.schemaId();
  if (schemaId != MessageHeader::sbeSchemaId())
  {
    throw ClusterException(
      "expected schemaId=" + std::to_string(MessageHeader::sbeSchemaId()) +
      ", actual=" + std::to_string(schemaId),
      SOURCEINFO);
  }

  
  std::uint16_t templateId = msgHeader.templateId();
  if (SessionMessageHeader::sbeTemplateId() == templateId)
  {
    SessionMessageHeader message(
      buffer.sbeData(),
      offset + MessageHeader::encodedLength(),
      static_cast<std::uint64_t>(length) - MessageHeader::encodedLength(),
      msgHeader.blockLength(),
      msgHeader.version());

    const std::uint64_t sessionId = message.clusterSessionId();
    if (sessionId == m_clusterSessionId)
    {
      m_onSessionMessage(
	sessionId,
	message.timestamp(),
	buffer,
	// TODO CHECK THIS
	offset + SessionMessageHeader::sbeBlockLength(),
	length - SessionMessageHeader::sbeBlockLength(),
	header);
    }
  }
  else if (SessionEvent::sbeTemplateId() == templateId)
  {
    SessionEvent event(
      buffer.sbeData(),
      offset + MessageHeader::encodedLength(),
      static_cast<std::uint64_t>(length) - MessageHeader::encodedLength(),
      msgHeader.blockLength(),
      msgHeader.version());

    const std::uint64_t sessionId = event.clusterSessionId();
    if (sessionId == m_clusterSessionId)
    {
      m_onSessionEvent(
	event.correlationId(),
	sessionId,
	event.leadershipTermId(),
	event.leaderMemberId(),
	event.code(),
	event.detail());
    }
  }
  else if (NewLeaderEvent::sbeTemplateId() == templateId)
  {
    NewLeaderEvent event(
      buffer.sbeData(),
      offset + MessageHeader::encodedLength(),
      static_cast<std::uint64_t>(length) - MessageHeader::encodedLength(),
      msgHeader.blockLength(),
      msgHeader.version());

    const std::uint64_t sessionId = event.clusterSessionId();
    if (sessionId == m_clusterSessionId)
    {
      m_onNewLeaderEvent(
	sessionId,
	event.leadershipTermId(),
	event.leaderMemberId(),
	event.ingressEndpoints());
    }
  }
  else if (AdminResponse::sbeTemplateId() == templateId)
  {
    AdminResponse response(
      buffer.sbeData(),
      offset + MessageHeader::encodedLength(),
      static_cast<std::uint64_t>(length) - MessageHeader::encodedLength(),
      msgHeader.blockLength(),
      msgHeader.version());

    std::uint64_t sessionId = response.clusterSessionId();
    if (sessionId == m_clusterSessionId)
    {
      util::index_t payloadOffset = response.offset() +
	AdminResponse::sbeBlockLength() +
	response.messageHeaderLength() +
	AdminResponse::payloadHeaderLength();

      util::index_t payloadLength = response.payloadLength();

      m_onAdminResponse(
	sessionId,
	response.correlationId(),
	response.requestType(),
	response.responseCode(),
	response.message(),
	buffer,
	payloadOffset,
	payloadLength);
    }
  }
}

}}}
