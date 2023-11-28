#ifndef AERON_CLUSTER_CONSENSUS_MODULE_PROXY_H
#define AERON_CLUSTER_CONSENSUS_MODULE_PROXY_H

#include <Aeron.h>
#include <cstdint>

namespace aeron { namespace cluster { namespace service {

/**
 * Proxy for communicating with the Consensus Module over IPC.
 * <p>
 * <b>Note: </b>This class is not for public use.
 */
class ConsensusModuleProxy
{
  explicit ConsensusModuleProxy(std::shared_ptr<ExclusivePublication> publication);

  bool closeSession(std::int64_t clusterSessionId);
  
  /**
 * Remove a member by id from the cluster.
 *
 * @param memberId  to be removed.
 * @param isPassive to indicate if the member is passive or not.
 * @return true of the request was successfully sent, otherwise false.
 */
  bool removeMember(std::int32_t memberId, bool isPassive);

  bool scheduleTimer(std::int64_t correlationId, std::int64_t deadline);

  bool cancelTimer(std::int64_t correlationId);

  bool ack(
    std::int64_t logPosition, std::int64_t timestamp, std::int64_t ackId,
    std::int64_t relevantId, std::int32_t serviceId);

  void close()
  {
    m_publication->close();
  }

  std::int64_t offer(AtomicBuffer& header, AtomicBuffer& buffer)
  {
    std::array<AtomicBuffer, 2> buffers;
    buffers[0] = header;
    buffers[1] = buffer;
    return m_publication->offer(buffers);
  }

private:
  std::shared_ptr<ExclusivePublication> m_publication;
};

}}}

#endif
