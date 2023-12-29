#include "ServiceSnapshotTaker.h"
#include "ClientSession.h"

#include "client/AeronArchive.h"

namespace aeron { namespace cluster { namespace service {

ServiceSnapshotTaker::ServiceSnapshotTaker(
    std::shared_ptr<ExclusivePublication> publication) :
    SnapshotTaker(publication)
{
}

}}}
