#include "SnapshotTaker.h"
#include "ClusterClock.h"
#include "client/ClusterException.h"
#include "client/AeronArchive.h"

namespace aeron { namespace cluster { namespace service {

using client::ClusterException;

void SnapshotTaker::checkResult(std::int64_t result)
{
  if (result == NOT_CONNECTED ||
      result == PUBLICATION_CLOSED ||
      result == MAX_POSITION_EXCEEDED)
  {
    throw ClusterException(std::string("unexpected publication state: ") + std::to_string(result), SOURCEINFO);
  }
}

SnapshotTaker::SnapshotTaker(
  std::shared_ptr<ExclusivePublication> publication) :
  m_publication(publication)
{
}

}}}
