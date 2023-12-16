#ifndef AERON_CLUSTER_SERVICE_CLUSTER_MARK_FILE_H
#define AERON_CLUSTER_SERVICE_CLUSTER_MARK_FILE_H

#include "Aeron.h"

#include "aeron_cluster_codecs_mark/ClusterComponentType.h"
#include "aeron_cluster_codecs_mark/MarkFileHeader.h"

#include <limits>

namespace aeron { namespace cluster { namespace service {

struct EpochClock
{
  virtual std::int64_t time() = 0;
};

class ClusterMarkFile
{
public:
  static constexpr int MAJOR_VERSION = 0;
  static constexpr int MINOR_VERSION = 3;
  static constexpr int PATCH_VERSION = 0;
  //constexpr int SEMANTIC_VERSION = SemanticVersion.compose(MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION);

  static constexpr int HEADER_LENGTH = 8 * 1024;
  static constexpr int VERSION_FAILED = -1;
  static constexpr std::int32_t ERROR_BUFFER_MIN_LENGTH = 1024 * 1024;
  static constexpr std::int32_t ERROR_BUFFER_MAX_LENGTH = std::numeric_limits<int32_t>::max() - HEADER_LENGTH;

  static constexpr const char* FILE_EXTENSION = ".dat";
  static constexpr const char* LINK_FILE_EXTENSION = ".lnk";
  static constexpr const char* FILENAME = "cluster-mark.dat";
  static constexpr const char* LINK_FILENAME = "cluster-mark.lnk";
  static constexpr const char* SERVICE_FILENAME_PREFIX = "cluster-mark-service-";

  static bool isServiceMarkFile(const std::string &path);

  /**
   * Determines if this path name matches the consensus module file name pattern.
   *
   * @param path       to examine.
   * @return true if the name matches.
   */
  static bool isConsensusModuleMarkFile(const std::string &path);

  ClusterMarkFile(
    const std::string &filename,
    codecs::mark::ClusterComponentType::Value type,
    std::int32_t errorBufferLength,
    EpochClock &epochClock,
    int64_t timeoutMs);

  /**
   * Get the current value of a candidate term id if a vote is placed in an election.
   *
   * @return the current candidate term id within an election after voting or {@link Aeron#NULL_VALUE} if
   * no voting phase of an election is currently active.
   */
  std::int64_t candidateTermId()
  {
    return m_buffer.getInt64Volatile(m_header.candidateTermIdEncodingOffset());
  }

  /**
   * Cluster member id either assigned statically or as the result of dynamic membership join.
   *
   * @return cluster member id either assigned statically or as the result of dynamic membership join.
   */
  std::int32_t memberId()
  {
    return m_buffer.getInt32(m_header.memberIdEncodingOffset());
  }

  /**
   * Member id assigned as part of dynamic join of a cluster.
   *
   * @param memberId assigned as part of dynamic join of a cluster.
   */
  void memberId(std::int32_t memberId)
  {
    m_buffer.putInt32(m_header.memberIdEncodingOffset(), memberId);
  }

  /**
   * Identity of the cluster instance so multiple clusters can run on the same driver.
   *
   * @return id of the cluster instance so multiple clusters can run on the same driver.
   */
  std::int32_t clusterId()
  {
    return m_buffer.getInt32(m_header.clusterIdEncodingOffset());
  }

  /**
   * Identity of the cluster instance so multiple clusters can run on the same driver.
   *
   * @param clusterId of the cluster instance so multiple clusters can run on the same driver.
   */
  void clusterId(std::int32_t clusterId)
  {
    m_buffer.putInt32(m_header.clusterIdEncodingOffset(), clusterId);
  }

private:
  AtomicBuffer m_buffer;
  std::shared_ptr<MemoryMappedFile> m_mapFile;
  codecs::mark::MarkFileHeader m_header;
};

}}}
#endif
