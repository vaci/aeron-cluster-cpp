#include "ConsensusModuleProxy.h"
#include "client/ClusterException.h"
#include "aeron_cluster_service/MessageHeader.h"
#include "aeron_cluster_client/BooleanType.h"
#include "aeron_cluster_client/CancelTimer.h"
#include "aeron_cluster_client/CloseSession.h"
#include "aeron_cluster_client/RemoveMember.h"
#include "aeron_cluster_client/ServiceAck.h"
#include "aeron_cluster_client/ScheduleTimer.h"

namespace aeron { namespace cluster { namespace service {

using BooleanType = client::BooleanType;
using ClusterException = client::ClusterException;

inline static void checkResult(std::int64_t result)
{
  if (result == NOT_CONNECTED ||
      result == PUBLICATION_CLOSED ||
      result == MAX_POSITION_EXCEEDED)
  {
    throw new ClusterException(std::string("unexpected publication state: ") + std::to_string(result), SOURCEINFO);
  }
}

template<typename Codec>
inline static Codec &wrapAndApplyHeader(Codec &codec, AtomicBuffer &buffer)
{
    return codec.wrapAndApplyHeader(buffer.sbeData(), 0, static_cast<std::uint64_t>(buffer.capacity()));
}


ConsensusModuleProxy::ConsensusModuleProxy(std::shared_ptr<ExclusivePublication> publication) :
  m_publication(publication)
{
}

bool ConsensusModuleProxy::closeSession(std::int64_t clusterSessionId)
{
  std::uint64_t length = MessageHeader::encodedLength() + client::CloseSession::sbeBlockLength();

  int attempts = 3;
  do
  {
    BufferClaim bufferClaim;
    std::int64_t result = m_publication->tryClaim(length, bufferClaim);
    if (result > 0)
    {
      auto buffer = bufferClaim.buffer();
      client::CloseSession request;
      wrapAndApplyHeader(request, buffer)
	.clusterSessionId(clusterSessionId);
      
      bufferClaim.commit();
      
      return true;
    }

    checkResult(result);
  }
  while (--attempts > 0);
  return false;

}

/**
 * Remove a member by id from the cluster.
 *
 * @param memberId  to be removed.
 * @param isPassive to indicate if the member is passive or not.
 * @return true of the request was successfully sent, otherwise false.
 */
bool ConsensusModuleProxy::removeMember(std::int32_t memberId, bool isPassive)
{
  std::uint64_t length = MessageHeader::encodedLength() + client::RemoveMember::sbeBlockLength();

  int attempts = 3;
  do
  {
    BufferClaim bufferClaim;
    std::int64_t result = m_publication->tryClaim(length, bufferClaim);
    if (result > 0)
    {
      auto buffer = bufferClaim.buffer();
      client::RemoveMember request;
      wrapAndApplyHeader(request, buffer)
	.memberId(memberId)
	.isPassive(isPassive ? BooleanType::Value::TRUE : BooleanType::Value::FALSE);
      
      bufferClaim.commit();
      
      return true;
    }

    checkResult(result);
  }
  while (--attempts > 0);
  return false;
}

bool ConsensusModuleProxy::scheduleTimer(std::int64_t correlationId, std::int64_t deadline)
{
  std::uint64_t length = MessageHeader::encodedLength() + client::ScheduleTimer::sbeBlockLength();

  int attempts = 3;
  do
  {
    BufferClaim bufferClaim;
    std::int64_t result = m_publication->tryClaim(length, bufferClaim);
    if (result > 0)
    {
      auto buffer = bufferClaim.buffer();
      client::ScheduleTimer request;
      wrapAndApplyHeader(request, buffer)
	.correlationId(correlationId)
	.deadline(deadline);
      
      bufferClaim.commit();
      
      return true;
    }

    checkResult(result);
  }
  while (--attempts > 0);
  return false;
}

bool ConsensusModuleProxy::cancelTimer(std::int64_t correlationId)
{
  std::uint64_t length = MessageHeader::encodedLength() + client::CancelTimer::sbeBlockLength();

  int attempts = 3;
  do
  {
    BufferClaim bufferClaim;
    std::int64_t result = m_publication->tryClaim(length, bufferClaim);
    if (result > 0)
    {
      auto buffer = bufferClaim.buffer();
      client::CancelTimer request;
      wrapAndApplyHeader(request, buffer)
	.correlationId(correlationId);
      
      bufferClaim.commit();
      
      return true;
    }

    checkResult(result);
  }
  while (--attempts > 0);
  return false;
}

bool ConsensusModuleProxy::ack(
  std::int64_t logPosition,
  std::int64_t timestamp,
  std::int64_t ackId,
  std::int64_t relevantId,
  std::int32_t serviceId)
{
  std::uint64_t length = MessageHeader::encodedLength() + client::ServiceAck::sbeBlockLength();

  int attempts = 3;
  do
  {
    BufferClaim bufferClaim;
    std::int64_t result = m_publication->tryClaim(length, bufferClaim);
    if (result > 0)
    {
      auto buffer = bufferClaim.buffer();
      client::ServiceAck request;
      wrapAndApplyHeader(request, buffer)
	.logPosition(logPosition)
	.timestamp(timestamp)
	.ackId(ackId)
	.relevantId(relevantId)
	.serviceId(serviceId);
      
      bufferClaim.commit();
      
      return true;
    }

    checkResult(result);
  }
  while (--attempts > 0);
  return false;
  
}

}}}
