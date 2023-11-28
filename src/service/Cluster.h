#ifndef AERON_CLUSTER_SERVICE_CLUSTER_H
#define AERON_CLUSTER_SERVICE_CLUSTER_H

#include "ClientSession.h"

#include <Aeron.h>
#include <cstdint>

namespace aeron { namespace cluster { namespace service {

class Cluster
{
  std::int32_t memberId() const;
  int role() const;
  std::int64_t logPosition() const;
  std::shared_ptr<Aeron> aeron();

  virtual std::shared_ptr<ClientSession> getClientSession(std::int64_t clusterSessionId);
  virtual bool closeClientSession(std::int64_t clusterSessionId) = 0;
};

}}}

#endif
