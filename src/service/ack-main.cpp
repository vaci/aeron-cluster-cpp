#include <Aeron.h>

#include "aeron_cluster_codecs/MessageHeader.h"
#include "aeron_cluster_codecs/ServiceAck.h"

using namespace aeron::cluster::codecs;

int main()
{
  aeron::Context aeronContext;
  aeron::Aeron aeron(aeronContext);

  auto pubId = aeron.addPublication("aeron:ipc", 105);
  auto pub = aeron.findPublication(pubId);
  while (pub == nullptr)
  {
    pub = aeron.findPublication(pubId);
    ::sleep(1);
  }

  
  std::uint64_t length = MessageHeader::encodedLength() + ServiceAck::sbeBlockLength();
  std::vector<unsigned char> buffer(length);

  ServiceAck request;
  request
    .wrapAndApplyHeader((char*)buffer.data(), 0, length)
    .logPosition(0)
    .timestamp(0)
    .ackId(0)
    .relevantId(aeron.clientId())
    .serviceId(0);


  auto result = pub->offer({buffer.data(), length});
  std::cout << result
	    << ":Sent ACK " << 0
	    << " of length " << length
	    << " to " << pub->channel() << ":" << pub->streamId()
	    << std::endl;
 
  while (true)
  {
    ::sleep(1);
  }
}
