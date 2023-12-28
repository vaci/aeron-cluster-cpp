#include "ServiceSnapshotTaker.h"
#include "ClientSession.h"

#include "aeron_cluster_codecs/MessageHeader.h"
#include "aeron_cluster_codecs/ClientSession.h"
#include "client/AeronArchive.h"
#include "client/ClusterException.h"

namespace aeron { namespace cluster { namespace service {

using namespace codecs;

using client::ClusterException;

ServiceSnapshotTaker::ServiceSnapshotTaker(
  std::shared_ptr<ExclusivePublication> publication) :
  SnapshotTaker(publication)
{
}

bool ServiceSnapshotTaker::snapshotSession(ClientSession &session)
{
  auto &responseChannel = session.responseChannel();
  auto &encodedPrincipal = session.encodedPrincipal();
  std::int32_t length = codecs::ClientSession::sbeBlockAndHeaderLength() +
    codecs::ClientSession::responseChannelHeaderLength() + responseChannel.length() +
    codecs::ClientSession::encodedPrincipalHeaderLength() + encodedPrincipal.size();

  std::uint8_t data[length];
  AtomicBuffer buffer(data, length);
  codecs::ClientSession message;
  message.wrapAndApplyHeader(buffer.sbeData(), 0, length)
    .clusterSessionId(session.id())
    .responseStreamId(session.responseStreamId())
    .putResponseChannel(responseChannel.c_str(), responseChannel.size())
    .putEncodedPrincipal(&encodedPrincipal[0], encodedPrincipal.size());
  std::int64_t result = m_publication->offer(buffer);
  if (result > 0)
  {
    return true;
  }
  checkResult(result);
  return false;

}

}}}
