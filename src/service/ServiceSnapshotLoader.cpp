#include "ServiceSnapshotLoader.h"
#include "ClusteredServiceAgent.h"
#include "aeron_cluster_codecs/MessageHeader.h"
#include "aeron_cluster_codecs/SnapshotMark.h"
#include "aeron_cluster_codecs/SnapshotMarker.h"
#include "aeron_cluster_codecs/ClientSession.h"
#include "client/ClusterException.h"

namespace aeron { namespace cluster { namespace service {

using namespace codecs;

using client::ClusterException;

namespace {

static controlled_poll_fragment_handler_t fragmentHandler(ServiceSnapshotLoader &loader)
{
  return
    [&](AtomicBuffer &buffer, util::index_t offset, util::index_t length, Header &header)
    {
      return loader.onFragment(buffer, offset, length, header);
    };
}

}

ServiceSnapshotLoader::ServiceSnapshotLoader(
  std::shared_ptr<Image> image,
  ClusteredServiceAgent &agent) :
  m_image(image),
  m_agent(agent)
{
}

std::int32_t ServiceSnapshotLoader::poll()
{
  return m_image->controlledPoll(fragmentHandler(*this), FRAGMENT_LIMIT);
}


ControlledPollAction ServiceSnapshotLoader::onFragment(AtomicBuffer buffer, util::index_t offset, util::index_t length, Header &header)
{
    using codecs::ClientSession;

    MessageHeader msgHeader(
	buffer.sbeData() + offset,
	static_cast<std::uint64_t>(length),
	MessageHeader::sbeSchemaVersion());
      
    auto schemaId = msgHeader.schemaId();
    if (schemaId != MessageHeader::sbeSchemaId())
    {
	throw ClusterException(std::string("expected schemaId=") + std::to_string(MessageHeader::sbeSchemaId()) + ", actual=" + std::to_string(schemaId), SOURCEINFO);
    }
	    
    switch (msgHeader.templateId())
    {
	case SnapshotMarker::sbeTemplateId():
	{
	    SnapshotMarker snapshotMarker(
		buffer.sbeData() + offset + MessageHeader::encodedLength(),
		length - MessageHeader::encodedLength(),
		msgHeader.blockLength(),
		msgHeader.version());

	    auto typeId = snapshotMarker.typeId();
	    if (typeId != Configuration::SNAPSHOT_TYPE_ID)
	    {
		throw ClusterException(
		    std::string("unexpected snapshot type: ") + std::to_string(typeId),
		    SOURCEINFO);
	    }
    
	    switch (snapshotMarker.mark())
	    {
		case SnapshotMark::Value::BEGIN:
		{
		    if (m_inSnapshot)
		    {
			throw ClusterException("already in snapshot", SOURCEINFO);
		    }
		    m_inSnapshot = true;
		    m_appVersion = snapshotMarker.appVersion();
		    // TODO
		    //m_timeUnit = ClusterClock.map(snapshotMarkerDecoder.timeUnit());
		    return ControlledPollAction::CONTINUE;
		}
		case SnapshotMark::Value::END:
		{
		    if (!m_inSnapshot)
		    {
			throw ClusterException("missing begin snapshot", SOURCEINFO);
		    }
		    m_isDone = true;
		    return ControlledPollAction::BREAK;
		}
		case SnapshotMark::Value::SECTION:
		case SnapshotMark::Value::NULL_VALUE:
		    break;
	    }
	    break;
	}

	case ClientSession::sbeTemplateId():
	{
	    ClientSession clientSession(
		buffer.sbeData() + offset + MessageHeader::encodedLength(),
		length - MessageHeader::encodedLength(),
		msgHeader.blockLength(),
		msgHeader.version());
	    
	    std::string responseChannel(clientSession.responseChannel(), clientSession.responseChannelLength());
	    std::vector<char> encodedPrincipal(
		clientSession.encodedPrincipal(),
		clientSession.encodedPrincipal() + clientSession.encodedPrincipalLength());
 
	    m_agent.addSession(
		clientSession.clusterSessionId(),
		clientSession.responseStreamId(),
		responseChannel,
		encodedPrincipal);

	    break;
	}
    }
  
    return ControlledPollAction::CONTINUE;
}

}}}
