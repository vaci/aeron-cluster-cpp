#ifndef AERON_CLUSTER_EGRESS_POLLER_H
#define AERON_CLUSTER_EGRESS_POLLER_H

#include "Aeron.h"
#include "ControlledFragmentAssembler.h"
#include "aeron_cluster_client/EventCode.h"

namespace aeron { namespace cluster { namespace client {

class EgressPoller
{
public:
  EgressPoller(
    std::shared_ptr<Subscription> subscription,
    int32_t fragmentLimit);


  std::shared_ptr<Subscription> subscription()
  {
    return m_subscription;
  }

  Image egressImage()
  {
    return *m_egressImage;
  }

  inline int32_t templateId() const
  {
    return m_templateId;
  }

  inline int64_t clusterSessionId() const
  {
    return m_clusterSessionId;
  }

  inline int64_t correlationId() const
  {
    return m_correlationId;
  }

  inline int64_t leadershipTermId() const
  {
    return m_leaderMemberId;
  }

  
  inline int64_t leaderMemberId() const
  {
    return m_leaderMemberId;
  }

  inline EventCode::Value eventCode() const
  {
    return m_eventCode;
  }

  inline int32_t version() const
  {
    return m_version;
  }

  inline const std::string &detail() const
  {
    return m_detail;
  }

  /**
   * Get the encoded challenge of the last challenge.
   *
   * @return the encoded challenge of the last challenge.
   */
  inline std::pair<const char *, std::uint32_t> encodedChallenge()
  {
    return m_encodedChallenge;
  }

  inline bool isPollComplete() const
  {
    return m_isPollComplete;
  }

  inline bool isChallenged() const
  {
    return m_isChallenged;
  }

  int32_t poll();

  ControlledPollAction onFragment(AtomicBuffer buffer, util::index_t offset, util::index_t length, Header& header);

private:
  std::shared_ptr<Subscription> m_subscription;
  int32_t m_fragmentLimit;
  ControlledFragmentAssembler m_fragmentAssembler;
  std::unique_ptr<Image> m_egressImage;
  int32_t m_templateId;
  int64_t m_clusterSessionId;
  int64_t m_correlationId;
  int64_t m_leadershipTermId;
  int32_t m_leaderMemberId;
  EventCode::Value m_eventCode;
  int32_t m_version;
  std::string m_detail;
  std::pair<const char *, std::uint32_t> m_encodedChallenge = { nullptr, 0 };
  bool m_isPollComplete;
  bool m_isChallenged;
};

}}}
 
#endif
