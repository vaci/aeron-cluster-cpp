#include "EgressPoller.h"

#include "ClusterException.h"
#include "aeron_cluster_client/Challenge.h"
#include "aeron_cluster_client/MessageHeader.h"
#include "aeron_cluster_client/SessionMessageHeader.h"
#include "aeron_cluster_client/SessionEvent.h"
#include "aeron_cluster_client/NewLeaderEvent.h"
#include "aeron_cluster_client/AdminResponse.h"

namespace aeron { namespace cluster { namespace client {

namespace {

static controlled_poll_fragment_handler_t fragmentHandler(EgressPoller &poller)
{
    return
        [&](AtomicBuffer &buffer, util::index_t offset, util::index_t length, Header &header)
        {
            return poller.onFragment(buffer, offset, length, header);
        };
}

}

EgressPoller::EgressPoller(
  std::shared_ptr<Subscription> subscription,
  int32_t fragmentLimit):
  m_subscription(subscription),
  m_fragmentLimit(fragmentLimit),
  m_fragmentAssembler(fragmentHandler(*this)),
  m_encodedChallenge(nullptr)
{
}

int EgressPoller::poll()
{
  if (m_isPollComplete)
  {
    m_isPollComplete = false;
    m_clusterSessionId = -1;
    m_correlationId = -1;
    m_leadershipTermId = -1;
    m_leaderMemberId = -1;
    m_templateId = -1;
    m_version = 0;
    m_eventCode = EventCode::Value::NULL_VALUE;
    m_detail = "";
    if (m_encodedChallenge != nullptr) {
      delete [] m_encodedChallenge;
      m_encodedChallenge = nullptr;
    }
  }

  //return m_subscription->poll(m_fragmentAssembler, m_fragmentLimit);
  return 0;
}

ControlledPollAction EgressPoller::onFragment(AtomicBuffer buffer, util::index_t offset, util::index_t length, Header& header)
{
  if (m_isPollComplete)
  {
    return ControlledPollAction::ABORT;
  }

  MessageHeader msgHeader(
    buffer.sbeData() + offset,
    static_cast<std::uint64_t>(length),
    MessageHeader::sbeSchemaVersion());

  const std::uint16_t schemaId = msgHeader.schemaId();
  if (schemaId != MessageHeader::sbeSchemaId())
  {
    throw new ClusterException(
      "expected schemaId=" + std::to_string(MessageHeader::sbeSchemaId()) + ", actual=" + std::to_string(schemaId),
      SOURCEINFO);
  }

  m_templateId = msgHeader.templateId();
  if (m_templateId == SessionMessageHeader::sbeTemplateId())
  {
    SessionMessageHeader message(
      buffer.sbeData(),
      offset + MessageHeader::encodedLength(),
      static_cast<std::uint64_t>(length) - MessageHeader::encodedLength(),
      msgHeader.blockLength(),
      msgHeader.version());

    m_leadershipTermId = message.leadershipTermId();
    m_clusterSessionId = message.clusterSessionId();
    m_isPollComplete = true;
    return ControlledPollAction::BREAK;
  }
  else if (m_templateId == SessionEvent::sbeTemplateId())
  {
    SessionEvent sessionEvent(
      buffer.sbeData(),
      offset + MessageHeader::encodedLength(),
      static_cast<std::uint64_t>(length) - MessageHeader::encodedLength(),
      msgHeader.blockLength(),
      msgHeader.version());
    m_clusterSessionId = sessionEvent.clusterSessionId();
    m_correlationId = sessionEvent.correlationId();
    m_leadershipTermId = sessionEvent.leadershipTermId();
    m_leaderMemberId = sessionEvent.leaderMemberId();
    m_eventCode = sessionEvent.code();
    m_version = sessionEvent.version();
    m_detail = sessionEvent.detail();
    m_isPollComplete = true;
    //m_egressImage = *reinterpret_cast<Image*>(header.context());
    return ControlledPollAction::BREAK;
  }
  else if (m_templateId == NewLeaderEvent::sbeTemplateId())
  {
    NewLeaderEvent newLeaderEvent(
      buffer.sbeData(),
      offset + MessageHeader::encodedLength(),
      static_cast<std::uint64_t>(length) - MessageHeader::encodedLength(),
      msgHeader.blockLength(),
      msgHeader.version());
    m_clusterSessionId = newLeaderEvent.clusterSessionId();
    m_leadershipTermId = newLeaderEvent.leadershipTermId();
    m_leaderMemberId = newLeaderEvent.leaderMemberId();
    m_detail = newLeaderEvent.ingressEndpoints();
    m_isPollComplete = true;
    return ControlledPollAction::BREAK;
  }
  else if (m_templateId == Challenge::sbeTemplateId())
  {
    Challenge challenge(
      buffer.sbeData(),
      offset + MessageHeader::encodedLength(),
      static_cast<std::uint64_t>(length) - MessageHeader::encodedLength(),
      msgHeader.blockLength(),
      msgHeader.version());
    m_encodedChallenge = new char[challenge.encodedChallengeLength()];
    challenge.getEncodedChallenge(m_encodedChallenge, challenge.encodedChallengeLength());
    m_clusterSessionId = challenge.clusterSessionId();
    m_correlationId = challenge.correlationId();
    m_isPollComplete = true;
    return ControlledPollAction::BREAK;
  }
  return ControlledPollAction::CONTINUE;
}

}}}
