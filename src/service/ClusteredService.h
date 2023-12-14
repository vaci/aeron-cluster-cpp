#ifndef AERON_CLUSTER_SERVICE_CLUSTERED_SERVICE_H
#define AERON_CLUSTER_SERVICE_CLUSTERED_SERVICE_H

#include <Aeron.h>
#include "Cluster.h"
#include "ClientSession.h"

namespace aeron { namespace cluster { namespace service {

// TODO
using CloseReason = int;

class ClusteredService
{
public:
  virtual ~ClusteredService();

  // onStart will be called repeatedly until it returns true?
  virtual bool onStart(Cluster &cluster, std::shared_ptr<Image> snapshotImage) = 0;

  /**
   * A session has been opened for a client to the cluster.
   *
   * @param session   for the client which have been opened.
   * @param timestamp at which the session was opened.
   */
  virtual void onSessionOpen(ClientSession &session, std::int64_t timestamp) = 0;

  /**
   * A session has been closed for a client to the cluster.
   *
   * @param session     that has been closed.
   * @param timestamp   at which the session was closed.
   * @param closeReason the session was closed.
   */
  virtual void onSessionClose(ClientSession &session, std::int64_t timestamp, CloseReason closeReason) = 0;

  /**
   * A message has been received to be processed by a clustered service.
   *
   * @param session   for the client which sent the message. This can be null if the client was a service.
   * @param timestamp for when the message was received.
   * @param buffer    containing the message.
   * @param offset    in the buffer at which the message is encoded.
   * @param length    of the encoded message.
   * @param header    aeron header for the incoming message.
   */
  virtual void onSessionMessage(
    ClientSession &session,
    std::int64_t timestamp,
    AtomicBuffer &buffer,
    util::index_t offset,
    util::index_t length,
    Header header) = 0;

  /**
   * A scheduled timer has expired.
   *
   * @param correlationId for the expired timer.
   * @param timestamp     at which the timer expired.
   */
  virtual void onTimerEvent(std::int64_t correlationId, std::int64_t timestamp) = 0;

  virtual bool onTakeSnapshot(std::shared_ptr<ExclusivePublication> snapshotPublication) = 0;

  /**
   * Notify that the cluster node has changed role.
   *
   * @param newRole that the node has assumed.
   */
  virtual void onRoleChange(Cluster::Role newRole) = 0;

  /**
   * Called when the container is going to terminate but only after a successful start.
   *
   * @param cluster with which the service can interact.
   */
  virtual void onTerminate(Cluster &cluster) = 0;

  /**
   * An election has been successful and a leader has entered a new term.
   *
   * @param leadershipTermId    identity for the new leadership term.
   * @param logPosition         position the log has reached as the result of this message.
   * @param timestamp           for the new leadership term.
   * @param termBaseLogPosition position at the beginning of the leadership term.
   * @param leaderMemberId      who won the election.
   * @param logSessionId        session id for the publication of the log.
   * @param timeUnit            for the timestamps in the coming leadership term.
   * @param appVersion          for the application configured in the consensus module.
   */
  virtual void onNewLeadershipTermEvent(
    std::int64_t leadershipTermId,
    std::int64_t logPosition,
    std::int64_t timestamp,
    std::int64_t termBaseLogPosition,
    std::int64_t leaderMemberId,
    std::int64_t logSessionId,
    // TODO int timeUnit,
    std::int32_t appVersion)
  {
  }

    
  virtual int doBackgroundWork(std::int64_t nowNs)
  {
    return 0;
  }
};

}}}

#endif
