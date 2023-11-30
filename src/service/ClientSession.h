#ifndef AERON_CLUSTER_SERVICE_CLIENT_SESSION_H
#define AERON_CLUSTER_SERVICE_CLIENT_SESSION_H

#include <Aeron.h>
#include <cstdint>

namespace aeron { namespace cluster { namespace service {

class ClientSession
{
public:
  virtual const std::string &responseChannel() const = 0;
  virtual std::int64_t id() const = 0;
  virtual const std::vector<char> &encodedPrincipal() = 0;
};

}}}

#endif
