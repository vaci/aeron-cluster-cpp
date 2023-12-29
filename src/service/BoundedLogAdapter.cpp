#include "BoundedLogAdapter.h"
#include "ClusteredServiceConfiguration.h"
#include "ClusteredServiceAgent.h"
#include "Cluster.h"
#include "client/ClusterException.h"
#include "client/AeronCluster.h"

#include "aeron_cluster_codecs/MessageHeader.h"
#include "aeron_cluster_codecs/SessionMessageHeader.h"
#include "aeron_cluster_codecs/NewLeadershipTermEvent.h"
#include "aeron_cluster_codecs/SessionOpenEvent.h"
#include "aeron_cluster_codecs/SessionCloseEvent.h"
#include "aeron_cluster_codecs/MembershipChangeEvent.h"
#include "aeron_cluster_codecs/TimerEvent.h"
#include "aeron_cluster_codecs/ClusterAction.h"
#include "aeron_cluster_codecs/ClusterActionRequest.h"

namespace aeron { namespace cluster { namespace service {

using client::ClusterException;

using namespace codecs;

namespace {

static aeron::controlled_poll_fragment_handler_t controlHandler(BoundedLogAdapter &adapter)
{
  return
    [&](AtomicBuffer &buffer, util::index_t offset, util::index_t length, Header &header)
    {
      return adapter.onFragment(buffer, offset, length, header);
    };
}

}

BoundedLogAdapter::BoundedLogAdapter(ClusteredServiceAgent& agent, int fragmentLimit) :
  m_agent(agent),
  m_fragmentLimit(fragmentLimit),
  m_fragmentAssembler(controlHandler(*this)),
  m_fragmentHandler(m_fragmentAssembler.handler())
{
}

void BoundedLogAdapter::close()
{
  if (m_image != nullptr)
  {
    m_image->close();
    m_image = nullptr;
  }
}

ControlledPollAction BoundedLogAdapter::onMessage(
  AtomicBuffer &buffer, util::index_t offset, util::index_t length, Header &header)
{
  MessageHeader msgHeader(
    buffer.sbeData() + offset,
    static_cast<std::uint64_t>(length),
    MessageHeader::sbeSchemaVersion());

  const std::int32_t schemaId = msgHeader.schemaId();
  if (schemaId != MessageHeader::sbeSchemaId())
  {
    throw ClusterException(
	"expected schemaId=" + std::to_string(MessageHeader::sbeSchemaId()) +
	", actual=" + std::to_string(schemaId),
	SOURCEINFO);
  }

  const std::int32_t templateId = msgHeader.templateId();
  if (templateId == SessionMessageHeader::sbeTemplateId())
  {
    SessionMessageHeader sessionHeader(
      buffer.sbeData() + offset + MessageHeader::encodedLength(),
      static_cast<std::uint64_t>(length) - MessageHeader::encodedLength(),
      msgHeader.blockLength(),
      msgHeader.version());

    m_agent.onSessionMessage(
      header.position(),
      sessionHeader.clusterSessionId(),
      sessionHeader.timestamp(),
      buffer,
      offset + SessionMessageHeader::sbeBlockAndHeaderLength(),
      length - SessionMessageHeader::sbeBlockAndHeaderLength(),
      header);

    return ControlledPollAction::CONTINUE;
  }

  switch (templateId)
  {
  case TimerEvent::sbeTemplateId():
    {
      TimerEvent timerEvent(
	buffer.sbeData() + offset + MessageHeader::encodedLength(),
	static_cast<std::uint64_t>(length) - MessageHeader::encodedLength(),
	msgHeader.blockLength(),
	msgHeader.version());
	    
      m_agent.onTimerEvent(
	header.position(),
	timerEvent.correlationId(),
	timerEvent.timestamp());
      break;
    }

  case SessionOpenEvent::sbeTemplateId():
    {
      SessionOpenEvent openEvent(
	buffer.sbeData() + offset + MessageHeader::encodedLength(),
	static_cast<std::uint64_t>(length) - MessageHeader::encodedLength(),
	msgHeader.blockLength(),
	msgHeader.version());

      auto responseChannel = openEvent.responseChannel();
      std::vector<char> encodedPrincipal;
      encodedPrincipal.resize(openEvent.encodedPrincipalLength());
      // TODO
      //final byte[] encodedPrincipal = new byte[openEventDecoder.encodedPrincipalLength()];
      //openEvent.getEncodedPrincipal(encodedPrincipal, 0, encodedPrincipal.length);
      
      m_agent.onSessionOpen(
	openEvent.leadershipTermId(),
	header.position(),
	openEvent.clusterSessionId(),
	openEvent.timestamp(),
	openEvent.responseStreamId(),
	responseChannel,
	encodedPrincipal);
      break;
    }
	  
  case SessionCloseEvent::sbeTemplateId():
    {
      SessionCloseEvent closeEvent(
	buffer.sbeData() + offset + MessageHeader::encodedLength(),
	static_cast<std::uint64_t>(length) - MessageHeader::encodedLength(),
	msgHeader.blockLength(),
	msgHeader.version());

      m_agent.onSessionClose(
	closeEvent.leadershipTermId(),
	header.position(),
	closeEvent.clusterSessionId(),
	closeEvent.timestamp(),
	closeEvent.closeReason());
      break;
    }
  case ClusterActionRequest::sbeTemplateId():
    {
      ClusterActionRequest actionRequest(
	buffer.sbeData() + offset + MessageHeader::encodedLength(),
	static_cast<std::uint64_t>(length) - MessageHeader::encodedLength(),
	msgHeader.blockLength(),
	msgHeader.version());

      // TODO
      //auto flags = ClusterActionRequest::flagsNullValue() != actionRequest.flags() ?
      //	actionRequest.flags() : ConsensusModule::CLUSTER_ACTION_FLAGS_DEFAULT;

      auto flags = Configuration::CLUSTER_ACTION_FLAGS_DEFAULT;
      m_agent.onServiceAction(
	actionRequest.leadershipTermId(),
	actionRequest.logPosition(),
	actionRequest.timestamp(),
	actionRequest.action(),
	flags);
      break;
    }
		
  case NewLeadershipTermEvent::sbeTemplateId():
    {
      NewLeadershipTermEvent newLeadershipTermEvent(
	buffer.sbeData() + offset + MessageHeader::encodedLength(),
	static_cast<std::uint64_t>(length) - MessageHeader::encodedLength(),
	msgHeader.blockLength(),
	msgHeader.version());
	    
      m_agent.onNewLeadershipTermEvent(
	newLeadershipTermEvent.leadershipTermId(),
	newLeadershipTermEvent.logPosition(),
	newLeadershipTermEvent.timestamp(),
	newLeadershipTermEvent.termBaseLogPosition(),
	newLeadershipTermEvent.leaderMemberId(),
	newLeadershipTermEvent.logSessionId(),
	// TODO ClusterClock.map(newLeadershipTermEventDecoder.timeUnit()),
	newLeadershipTermEvent.appVersion());
      break;
    }

  case MembershipChangeEvent::sbeTemplateId():
    {
      MembershipChangeEvent membershipChangeEvent(
	buffer.sbeData() + offset + MessageHeader::encodedLength(),
	static_cast<std::uint64_t>(length) - MessageHeader::encodedLength(),
	msgHeader.blockLength(),
	msgHeader.version());

      m_agent.onMembershipChange(
	membershipChangeEvent.logPosition(),
	membershipChangeEvent.timestamp(),
	membershipChangeEvent.changeType(),
	membershipChangeEvent.memberId());
      break;
    }
  }
  return ControlledPollAction::CONTINUE;
}

ControlledPollAction BoundedLogAdapter::onFragment(AtomicBuffer &buffer, util::index_t offset, util::index_t length, Header &header)
{
  using namespace aeron::concurrent::logbuffer::FrameDescriptor;

  auto action = ControlledPollAction::CONTINUE;
  auto flags = header.flags();

  if ((flags & UNFRAGMENTED) == UNFRAGMENTED)
  {
    action = onMessage(buffer, offset, length, header);
  }
  else if ((flags & BEGIN_FRAG) == BEGIN_FRAG)
  {
    m_builder.reset();
    m_builder.append(buffer, offset, length, header);
    auto nextOffset = BitUtil::align(
      offset + length + DataFrameHeader::LENGTH, FrameDescriptor::FRAME_ALIGNMENT);
    m_builder.nextTermOffset(nextOffset);
  }
  else if (offset == m_builder.nextTermOffset())
  {
    auto limit = m_builder.limit();
    
    m_builder.append(buffer, offset, length, header);

    if ((flags & END_FRAG) == END_FRAG)
    {
      // TODO fix length
      AtomicBuffer buffer(m_builder.buffer(), m_builder.limit());
      action = onMessage(buffer, 0, m_builder.limit(), header);
      
      if (ControlledPollAction::ABORT == action)
      {
	m_builder.limit(limit);
      }
      else
      {
	m_builder.reset();
      }
    }
    else
    {
      auto nextOffset = BitUtil::align(
	offset + length + DataFrameHeader::LENGTH, FrameDescriptor::FRAME_ALIGNMENT);
      m_builder.nextTermOffset(nextOffset);
    }
  }
  else
  {
    m_builder.reset();
  }
  
  return action;
}

}}}
