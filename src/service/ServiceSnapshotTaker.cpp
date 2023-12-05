#include "ServiceSnapshotTaker.h"
#include "ClientSession.h"

#include "aeron_cluster_service/MessageHeader.h"
#include "aeron_cluster_client/ClientSession.h"
#include "client/AeronArchive.h"
#include "client/ClusterException.h"

namespace aeron { namespace cluster { namespace service {

using ClientSessionMessage = client::ClientSession;
using client::ClusterException;

static inline void checkResult(std::int64_t result)
{
  if (result == NOT_CONNECTED ||
      result == PUBLICATION_CLOSED ||
      result == MAX_POSITION_EXCEEDED)
  {
    throw ClusterException(std::string("unexpected publication state: ") + std::to_string(result), SOURCEINFO);
  }
}

ServiceSnapshotTaker::ServiceSnapshotTaker(
  std::shared_ptr<ExclusivePublication> publication, std::shared_ptr<Aeron> aeron) :
  SnapshotTaker(publication, aeron)
{
}


bool ServiceSnapshotTaker::snapshotSession(ClientSession &session)
{
  auto& responseChannel = session.responseChannel();
  auto& encodedPrincipal = session.encodedPrincipal();
  std::int32_t length = ClientSessionMessage::sbeBlockAndHeaderLength() +
    ClientSessionMessage::responseChannelHeaderLength() + responseChannel.length() +
    ClientSessionMessage::encodedPrincipalHeaderLength() + encodedPrincipal.size();

  if (length <= m_publication->maxPayloadLength())
  {
    BufferClaim bufferClaim;
    std::int64_t result = m_publication->tryClaim(length, bufferClaim);
    if (result > 0)
    {
      auto buffer = bufferClaim.buffer();
      util::index_t offset = bufferClaim.offset();

      // TODO
      //encodeSession(session, responseChannel, encodedPrincipal, buffer, offset);
      bufferClaim.commit();
      return true;
    }

    checkResult(result);
    return false;
  }
  else
  {
    util::index_t offset = 0;
    std::uint8_t data[1024];
    AtomicBuffer buffer(data, 1024);
    encodeSession(session, responseChannel, encodedPrincipal, buffer);
    std::int64_t result = m_publication->offer(buffer);
    if (result > 0)
    {
      return true;
    }
    checkResult(result);
    return false;
  }
}

void ServiceSnapshotTaker::encodeSession(
  ClientSession &session,
  const std::string &responseChannel,
  const std::vector<char> &encodedPrincipal,
  AtomicBuffer &buffer)
{
  MessageHeader header;

  ClientSessionMessage message;
  message
    .wrapAndApplyHeader(reinterpret_cast<char*>(buffer.buffer()), 0, buffer.capacity())
    .clusterSessionId(session.id())
    .responseStreamId(session.responseStreamId())
    .putResponseChannel(responseChannel.c_str(), responseChannel.size())
    .putEncodedPrincipal(&encodedPrincipal[0], encodedPrincipal.size());
}

}}}
