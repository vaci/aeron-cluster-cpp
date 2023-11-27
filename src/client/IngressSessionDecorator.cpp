#include "IngressSessionDecorator.h"

namespace aeron { namespace cluster { namespace client {


IngressSessionDecorator::IngressSessionDecorator(
  std::int64_t clusterSessionId,
  std::uint64_t leadershipTermId)
{
  m_message.wrapAndApplyHeader(m_headerBuffer, 0, 0)
    .leadershipTermId(leadershipTermId)
    .clusterSessionId(clusterSessionId)
    .timestamp(NULL_VALUE);
}

}}}
