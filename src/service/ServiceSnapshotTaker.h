#ifndef AERON_CLUSTER_SERVICE_SERVICE_SNAPSHOT_TAKER_H
#define AERON_CLUSTER_SERVICE_SERVICE_SNAPSHOT_TAKER_H

#include <Aeron.h>
#include "ClientSession.h"
#include "SnapshotTaker.h"
#include "aeron_cluster_codecs/MessageHeader.h"
#include "aeron_cluster_codecs/ClientSession.h"

namespace aeron { namespace cluster { namespace service {

class ClientSession;

class ServiceSnapshotTaker : public SnapshotTaker
{
public:
  explicit ServiceSnapshotTaker(std::shared_ptr<ExclusivePublication>);

  template <typename IdleStrategy = aeron::concurrent::YieldingIdleStrategy>
  void snapshotSession(ClientSession &session);
};

template <typename IdleStrategy>
inline void ServiceSnapshotTaker::snapshotSession(ClientSession &session)
{
    using ClientSessionMessage = codecs::ClientSession;

    auto &responseChannel = session.responseChannel();
    auto &encodedPrincipal = session.encodedPrincipal();
    std::int32_t length = ClientSessionMessage::sbeBlockAndHeaderLength() +
	ClientSessionMessage::responseChannelHeaderLength() + responseChannel.length() +
	ClientSessionMessage::encodedPrincipalHeaderLength() + encodedPrincipal.size();

    std::uint8_t data[length];
    AtomicBuffer buffer(data, length);
    ClientSessionMessage message;
    message.wrapAndApplyHeader(buffer.sbeData(), 0, length)
	.clusterSessionId(session.id())
	.responseStreamId(session.responseStreamId())
	.putResponseChannel(responseChannel.c_str(), responseChannel.size())
	.putEncodedPrincipal(&encodedPrincipal[0], encodedPrincipal.size());

    offer<IdleStrategy>(buffer, 0, length);
}

}}}

#endif
