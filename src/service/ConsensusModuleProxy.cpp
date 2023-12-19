#include "ConsensusModuleProxy.h"
#include "client/ClusterException.h"
#include "aeron_cluster_codecs/MessageHeader.h"
#include "aeron_cluster_codecs/BooleanType.h"
#include "aeron_cluster_codecs/CancelTimer.h"
#include "aeron_cluster_codecs/CloseSession.h"
#include "aeron_cluster_codecs/RemoveMember.h"
#include "aeron_cluster_codecs/ServiceAck.h"
#include "aeron_cluster_codecs/ScheduleTimer.h"

namespace aeron { namespace cluster { namespace service {

using namespace codecs;

using ClusterException = client::ClusterException;

inline static void checkResult(std::int64_t result)
{
  if (result == NOT_CONNECTED ||
      result == PUBLICATION_CLOSED ||
      result == MAX_POSITION_EXCEEDED)
  {
    throw ClusterException(std::string("unexpected publication state: ") + std::to_string(result), SOURCEINFO);
  }
}

template <typename Codec, typename Func>
bool offer(std::shared_ptr<ExclusivePublication> publication, Func func)
{
  auto length = Codec::sbeBlockAndHeaderLength();
  unsigned char data[length];
  AtomicBuffer buffer(data, length);
    
  Codec request{};
  request.wrapAndApplyHeader(buffer.sbeData(), 0, length);
  func(request);

  int attempts = 3;
  do
  {
    std::int64_t result = publication->offer(buffer);
    if (result > 0) {
      return true;
    }

    checkResult(result);
  }
  while (--attempts > 0);
  return false;
}

template <typename Codec, typename Func>
bool claim(std::shared_ptr<ExclusivePublication> publication, Func func)
{

  auto length = Codec::sbeBlockAndHeaderLength();

  int attempts = 3;
  do
  {
    BufferClaim bufferClaim;
    std::int64_t result = publication->tryClaim(length, bufferClaim);
    if (result > 0)
    {
      std::cout << "length: " << length << std::endl;
      std::cout << "result: " << result << std::endl; 
      std::cout << "bufoff: " << bufferClaim.offset() << std::endl; 
      std::cout << "buflen: " << bufferClaim.length() << std::endl;
      Codec request{};
      request.wrapAndApplyHeader(
	bufferClaim.buffer().sbeData() + bufferClaim.offset(),
	0,
	bufferClaim.length());
      func(request);
      bufferClaim.commit();
      return true;
    }

    checkResult(result);
  }
  while (--attempts > 0);
  return false;
}

ConsensusModuleProxy::ConsensusModuleProxy(std::shared_ptr<ExclusivePublication> publication) :
  m_publication(std::move(publication))
{
}

bool ConsensusModuleProxy::closeSession(std::int64_t clusterSessionId)
{
  return claim<CloseSession>(m_publication, [&](auto &request) {
    request.clusterSessionId(clusterSessionId);
  });
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
  return claim<RemoveMember>(m_publication, [&](auto &request) {
    request
      .memberId(memberId)
      .isPassive(isPassive ? BooleanType::Value::TRUE : BooleanType::Value::FALSE);
  });
}

bool ConsensusModuleProxy::scheduleTimer(std::int64_t correlationId, std::int64_t deadline)
{
  return claim<ScheduleTimer>(m_publication, [&](auto &request) {
    request
      .correlationId(correlationId)
      .deadline(deadline);
  });
}

bool ConsensusModuleProxy::cancelTimer(std::int64_t correlationId)
{
  return claim<CancelTimer>(m_publication, [&](auto &request) {
      request.correlationId(correlationId);
  });
}

bool ConsensusModuleProxy::ack(
  std::int64_t logPosition,
  std::int64_t timestamp,
  std::int64_t ackId,
  std::int64_t relevantId,
  std::int32_t serviceId)
{
  /*
  std::uint64_t length = MessageHeader::encodedLength() + ServiceAck::sbeBlockLength();

  unsigned char data[length];
  AtomicBuffer buffer{&data[0], length};
  ServiceAck request;
  wrapAndApplyHeader(request, buffer)
    .logPosition(logPosition)
    .timestamp(timestamp)
    .ackId(ackId)
    .relevantId(relevantId)
    .serviceId(serviceId);

  auto result = m_publication->offer(buffer);
  if (result > 0)
  {
    return true;
  }
  checkResult(result);
  return false;
  */
  
  return offer<ServiceAck>(m_publication, [&](auto &request) {
      request
      	.logPosition(logPosition)
	.timestamp(timestamp)
	.ackId(ackId)
	.relevantId(relevantId)
	.serviceId(serviceId);
  });

  /*
  int attempts = 3;
  do
  {
    BufferClaim bufferClaim;
    std::int64_t result = m_publication->tryClaim(length, bufferClaim);
    if (result > 0)
    {
      ServiceAck request;
      wrapAndApplyHeader(request, bufferClaim.buffer())
	.logPosition(logPosition)
	.timestamp(timestamp)
	.ackId(ackId)
	.relevantId(relevantId)
	.serviceId(serviceId);
      
      bufferClaim.commit();

      std::cout << result
		<< ":Sent ACK " << ackId
		<< " of length " << length
		<< " to " << m_publication->channel() << ":" << m_publication->streamId()
		<< std::endl;
      return true;
    }

    checkResult(result);
  }
  while (--attempts > 0);
  return false;
  */
}

}}}
