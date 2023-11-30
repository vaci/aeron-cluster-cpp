#include "ServiceSnapshotTaker.h"
#include "ClientSession.h"

#include "aeron_cluster_client/ClientSession.h"
#include "client/AeronArchive.h"

namespace aeron { namespace cluster { namespace service {

using ClientSessionMessage = client::ClientSession;

ServiceSnapshotTaker::ServiceSnapshotTaker(
  std::shared_ptr<ExclusivePublication> publication, std::shared_ptr<Aeron> aeron) :
  SnapshotTaker(publication, aeron)
{
}


void ServiceSnapshotTaker::snapshotSession(ClientSession &session)
{
  aeron::concurrent::BackoffIdleStrategy idle;
  auto& responseChannel = session.responseChannel();
  auto& encodedPrincipal = session.encodedPrincipal();
  std::int32_t length = ClientSessionMessage::sbeBlockAndHeaderLength() +
    ClientSessionMessage::responseChannelHeaderLength() + responseChannel.length() +
    ClientSessionMessage::encodedPrincipalHeaderLength() + encodedPrincipal.size();

  if (length <= m_publication->maxPayloadLength())
  {
    while (true)
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
	break;
      }

      checkResultAndIdle(result);
    }
  }
  else
  {
    util::index_t offset = 0;
    // TODO
    //encodeSession(session, responseChannel, encodedPrincipal, offerBuffer, offset);
    //offer(offerBuffer, offset, length);
  }
}

}}}
