#ifndef AERON_CLUSTER_SERVICE_CLUSTER_H
#define AERON_CLUSTER_SERVICE_CLUSTER_H

#include "ClientSession.h"
#include "client/ClusterException.h"

#include <Aeron.h>
#include <cstdint>

namespace aeron { namespace cluster { namespace service {

using client::ClusterException;

class Cluster
{
public:
  enum class Role : int32_t
  {
    FOLLOWER = 0,
    CANDIDATE = 1,
    LEADER = 2
  };

  static inline Role getRole(std::int32_t code)
  {
    if (code < 0 || code > 2)
    {
      throw ClusterException(std::string("Invalid role counter code: ") + std::to_string(code), SOURCEINFO);
    }
    
    return static_cast<Role>(code);
  }
  
  virtual ~Cluster();

  std::int32_t memberId() const;
  Role role() const;
  std::int64_t logPosition() const;
  std::shared_ptr<Aeron> aeron();

  virtual ClientSession* getClientSession(std::int64_t clusterSessionId) = 0;
  virtual bool closeClientSession(std::int64_t clusterSessionId) = 0;
};

}}}

#endif
