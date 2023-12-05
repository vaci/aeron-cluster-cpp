#ifndef AERON_CLUSTER_SERVICE_SERVICE_SNAPSHOT_TAKER_H
#define AERON_CLUSTER_SERVICE_SERVICE_SNAPSHOT_TAKER_H

#include <Aeron.h>
#include "SnapshotTaker.h"

namespace aeron { namespace cluster { namespace service {

class ClientSession;

class ServiceSnapshotTaker : public SnapshotTaker
{
public:
  explicit ServiceSnapshotTaker(std::shared_ptr<ExclusivePublication>, std::shared_ptr<Aeron> aeron);

  bool snapshotSession(ClientSession &session);

private:
  void encodeSession(
    ClientSession &session,
    const std::string &responseChannel,
    const std::vector<char> &encodedPrincipal,
    AtomicBuffer &buffer);
};

}}}

#endif
