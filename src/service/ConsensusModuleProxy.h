#ifndef AERON_CLUSTER_CONSENSUS_MODULE_PROXY_H
#define AERON_CLUSTER_CONSENSUS_MODULE_PROXY_H

#include <Aeron.h>
#include "concurrent/YieldingIdleStrategy.h"
#include "aeron_cluster_codecs/BooleanType.h"
#include "aeron_cluster_codecs/CancelTimer.h"
#include "aeron_cluster_codecs/CloseSession.h"
#include "aeron_cluster_codecs/RemoveMember.h"
#include "aeron_cluster_codecs/ScheduleTimer.h"
#include "aeron_cluster_codecs/ServiceAck.h"

#include <cstdint>

namespace aeron { namespace cluster { namespace service {

/**
 * Proxy for communicating with the Consensus Module over IPC.
 * <p>
 * <b>Note: </b>This class is not for public use.
 */
class ConsensusModuleProxy
{
public:
    constexpr static int ATTEMPTS = 3;

    explicit ConsensusModuleProxy(std::shared_ptr<ExclusivePublication> publication);

    inline std::shared_ptr<ExclusivePublication> publication()
    {
	return m_publication;
    }

    inline void close()
    {
	m_publication->close();
    }

    /**
     * Remove a member by id from the cluster.
     *
     * @param memberId  to be removed.
     * @param isPassive to indicate if the member is passive or not.
     * @return true of the request was successfully sent, otherwise false.
     */
    template<typename IdleStrategy = aeron::concurrent::YieldingIdleStrategy>
    inline bool removeMember(std::int32_t memberId, bool isPassive)
    {
	using codecs::BooleanType;

	return claim<codecs::RemoveMember, IdleStrategy>([&](auto &request) {
	    request.memberId(memberId);
	    request.isPassive(isPassive ? BooleanType::Value::TRUE : BooleanType::Value::FALSE);
	});
    }

    template<typename IdleStrategy = aeron::concurrent::YieldingIdleStrategy>
    inline bool scheduleTimer(std::int64_t correlationId, std::int64_t deadline)
    {
	return claim<codecs::ScheduleTimer, IdleStrategy>([&](auto &request) {
	    request.correlationId(correlationId);
	    request.deadline(deadline);
	});
    }

    template<typename IdleStrategy = aeron::concurrent::YieldingIdleStrategy>
    inline bool cancelTimer(std::int64_t correlationId)
    {
	return claim<codecs::CancelTimer, IdleStrategy>([&](auto &request) {
	    request.correlationId(correlationId);
	});
    }

    template<typename IdleStrategy = aeron::concurrent::YieldingIdleStrategy>
    inline bool closeSession(std::int64_t clusterSessionId)
    {
	return claim<codecs::CloseSession, IdleStrategy>([&](auto &request) {
	    request.clusterSessionId(clusterSessionId);
	});
    }

    template<typename IdleStrategy = aeron::concurrent::YieldingIdleStrategy>
    inline bool ack(
	std::int64_t logPosition, std::int64_t timestamp, std::int64_t ackId,
	std::int64_t relevantId, std::int32_t serviceId)
    {
	return claim<codecs::ServiceAck, IdleStrategy>([&](auto &request) {
	    request.logPosition(logPosition);
	    request.timestamp(timestamp);
	    request.ackId(ackId);
	    request.relevantId(relevantId);
	    request.serviceId(serviceId);
	});
    }

private:
    std::shared_ptr<ExclusivePublication> m_publication;

    static void checkResult(std::int64_t result);

    template <typename Idle>
    inline bool tryClaim(BufferClaim &bufferClaim, util::index_t length)
    {
	Idle idle;
	int attempts = ATTEMPTS;

	do
	{
	    std::int64_t result = m_publication->tryClaim(length, bufferClaim);    
	    if (result > 0)
	    {
		return true;
	    }
	    checkResult(result);
	    idle.idle();
	}
	while (--attempts > 0);
	return false;
    }

    template <typename Codec, typename Idle, typename Func>
    inline bool claim(Func func, util::index_t length = Codec::sbeBlockAndHeaderLength())
    {
	BufferClaim bufferClaim;
	if (tryClaim<Idle>(bufferClaim, length))
	{
	    Codec request;
	    request.wrapAndApplyHeader(
		bufferClaim.buffer().sbeData() + bufferClaim.offset(),
		0,
		bufferClaim.length());
	    func(request);
	    bufferClaim.commit();
	    return true;
	}
	return false;
    }
};

}}}

#endif
