#ifndef AERON_CLUSTER_SERVICE_CLUSTERED_SERVICE_AGENT_H
#define AERON_CLUSTER_SERVICE_CLUSTERED_SERVICE_AGENT_H

#include <Aeron.h>
#include <cstdint>
#include "ClientSession.h"
#include "Cluster.h"

namespace aeron { namespace cluster { namespace service {

class ClusteredServiceAgent : public Cluster
{
public:
  std::shared_ptr<ClientSession> getClientSession(std::int64_t clusterSessionId) override;
  bool closeClientSession(std::int64_t clusterSessionId) override;
};

}}}

#endif
