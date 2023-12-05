#include "SnapshotTaker.h"
#include "ClusterClock.h"
#include "client/ClusterException.h"
#include "client/AeronArchive.h"
#include "aeron_cluster_client/SnapshotMarker.h"
#include "aeron_cluster_client/ClusterTimeUnit.h"

namespace aeron { namespace cluster { namespace service {

using client::SnapshotMarker;
using client::ClusterTimeUnit;
using client::ClusterException;

namespace {

static inline void checkResult(std::int64_t result)
{
  if (result == NOT_CONNECTED ||
      result == PUBLICATION_CLOSED ||
      result == MAX_POSITION_EXCEEDED)
  {
    throw ClusterException(std::string("unexpected publication state: ") + std::to_string(result), SOURCEINFO);
  }
}

}

SnapshotTaker::SnapshotTaker(
  std::shared_ptr<ExclusivePublication> publication,
  std::shared_ptr<Aeron> aeron) :
  m_publication(publication),
  m_aeron(aeron)
{
}

bool SnapshotTaker::markSnapshot(
  std::int64_t snapshotTypeId,
  std::int64_t logPosition,
  std::int64_t leadershipTermId,
  std::int32_t  snapshotIndex,
  SnapshotMark::Value snapshotMark,
  std::int32_t  appVersion)
{

  BufferClaim bufferClaim;
  std::int64_t result = m_publication->tryClaim(SnapshotMarker::sbeBlockAndHeaderLength(), bufferClaim);
  if (result > 0)
  {
    auto buffer = bufferClaim.buffer();
    SnapshotMarker marker;
    marker
      .wrapAndApplyHeader(reinterpret_cast<char*>(buffer.buffer()), 0, bufferClaim.length())
      .typeId(snapshotTypeId)
      .logPosition(logPosition)
      .leadershipTermId(leadershipTermId)
      .index(snapshotIndex)
      .mark(snapshotMark)
      .timeUnit(ClusterTimeUnit::Value::NANOS)
      .appVersion(appVersion);
	
    bufferClaim.commit();
    return true;
  }
  checkResult(result);
  return false;
}

}}}
