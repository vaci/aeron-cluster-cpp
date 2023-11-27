#ifndef AERON_CLUSTER_EGRESS_ADAPTER_H
#define AERON_CLUSTER_EGRESS_ADAPTER_H

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

  //std::shared_ptr<Image> egressImage()
  //{
  //  return m_egressImage;
  // }

  int32_t templateId() const
  {
    return m_templateId;
  }

  int64_t clusterSessionId() const
  {
    return m_clusterSessionId;
  }

  int64_t correlationId() const
  {
    return m_correlationId;
  }

  int64_t leaderMemberId() const
  {
    return m_leaderMemberId;
  }

  EventCode::Value eventCode() const
  {
    return m_eventCode;
  }

  int32_t version() const
  {
    return m_version;
  }

  const std::string& detail() const
  {
    return m_detail;
  }

  const char* encodedChallenge() const
  {
    return m_encodedChallenge;
  }

  bool isPollComplete() const
  {
    return m_isPollComplete;
  }

  bool isChallenged() const
  {
    return m_isChallenged;
  }

  int32_t poll();

  ControlledPollAction onFragment(AtomicBuffer buffer, util::index_t offset, util::index_t length, Header& header);
private:
  std::shared_ptr<Subscription> m_subscription;
  int32_t m_fragmentLimit;
  ControlledFragmentAssembler m_fragmentAssembler;
  //std::shared_ptr<Image> m_egressImage;
  int32_t m_templateId;
  int64_t m_clusterSessionId;
  int64_t m_correlationId;
  int64_t m_leadershipTermId;
  int32_t m_leaderMemberId;
  EventCode::Value m_eventCode;
  int32_t m_version;
  std::string m_detail;
  char* m_encodedChallenge;
  bool m_isPollComplete;
  bool m_isChallenged;
};

}}}
 
#endif
