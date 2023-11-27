#ifndef AERON_CLUSTER_AERON_CLUSTER_H
#define AERON_CLUSTER_AERON_CLUSTER_H

#include "Aeron.h"
#include "aeron_cluster_client/MessageHeader.h"
#include "aeron_cluster_client/SessionMessageHeader.h"
#include "ControlledEgressListener.h"
#include "EgressPoller.h"
#include "EgressListener.h"

#include <map>

namespace aeron { namespace cluster { namespace client {

class AeronCluster
{
public:
  /**
   * Length of a session message header for cluster ingress or egress.
   */
  constexpr static std::uint64_t SESSION_HEADER_LENGTH =
    MessageHeader::encodedLength() + SessionMessageHeader::sbeBlockLength();

  struct Context
  {
    /**
     * Set the message timeout in nanoseconds to wait for sending or receiving a message.
     *
     * @param messageTimeoutNs to wait for sending or receiving a message.
     * @return this for a fluent API.
     * @see Configuration#MESSAGE_TIMEOUT_PROP_NAME
     */
    Context& messageTimeoutNs(std::int64_t messageTimeoutNs)
    {
      m_messageTimeoutNs = messageTimeoutNs;
      return *this;
    }

    /**
     * The message timeout in nanoseconds to wait for sending or receiving a message.
     *
     * @return the message timeout in nanoseconds to wait for sending or receiving a message.
     * @see Configuration#MESSAGE_TIMEOUT_PROP_NAME
     */
    std::int64_t messageTimeoutNs() const
    {
      return 0;
      //return CommonContext.checkDebugTimeout(messageTimeoutNs, TimeUnit.NANOSECONDS);
    }

    /**
     * The endpoints representing members for use with unicast to be substituted into the {@link #ingressChannel()}
     * for endpoints. A null value can be used when multicast where the {@link #ingressChannel()} contains the
     * multicast endpoint.
     *
     * @param clusterMembers which are all candidates to be leader.
     * @return *this for a fluent API.
     * @see Configuration#INGRESS_ENDPOINTS_PROP_NAME
     */
    Context &ingressEndpoints(const std::string &clusterMembers)
    {
      m_ingressEndpoints = clusterMembers;
      return *this;
    }

    /**
     * The endpoints representing members for use with unicast to be substituted into the {@link #ingressChannel()}
     * for endpoints. A null value can be used when multicast where the {@link #ingressChannel()} contains the
     * multicast endpoint.
     *
     * @return member endpoints of the cluster which are all candidates to be leader.
     * @see Configuration#INGRESS_ENDPOINTS_PROP_NAME
     */
    const std::string &ingressEndpoints() const
    {
      return m_ingressEndpoints;
    }

    /**
     * Set the channel parameter for the ingress channel.
     * <p>
     * The endpoints representing members for use with unicast are substituted from {@link #ingressEndpoints()}
     * for endpoints. If this channel contains a multicast endpoint, then {@link #ingressEndpoints()} should
     * be set to null.
     *
     * @param channel parameter for the ingress channel.
     * @return *this for a fluent API.
     * @see Configuration#INGRESS_CHANNEL_PROP_NAME
     */
    Context &ingressChannel(const std::string &channel)
    {
      m_ingressChannel = channel;
      return *this;
    }

    /**
     * Get the channel parameter for the ingress channel.
     * <p>
     * The endpoints representing members for use with unicast are substituted from {@link #ingressEndpoints()}
     * for endpoints. A null value can be used when multicast where this contains the multicast endpoint.
     *
     * @return the channel parameter for the ingress channel.
     * @see Configuration#INGRESS_CHANNEL_PROP_NAME
     */
    const std::string &ingressChannel() const
    {
      return m_ingressChannel;
    }

    /**
     * Set the stream id for the ingress channel.
     *
     * @param streamId for the ingress channel.
     * @return *this for a fluent API
     * @see Configuration#INGRESS_STREAM_ID_PROP_NAME
     */
    Context &ingressStreamId(std::int32_t streamId)
    {
      m_ingressStreamId = streamId;
      return *this;
    }

    /**
     * Get the stream id for the ingress channel.
     *
     * @return the stream id for the ingress channel.
     * @see Configuration#INGRESS_STREAM_ID_PROP_NAME
     */
    std::int32_t ingressStreamId() const
    {
      return m_ingressStreamId;
    }

    /**
     * Set the channel parameter for the egress channel.
     *
     * @param channel parameter for the egress channel.
     * @return *this for a fluent API.
     * @see Configuration#EGRESS_CHANNEL_PROP_NAME
     */
    Context &egressChannel(const std::string &channel)
    {
      m_egressChannel = channel;
      return *this;
    }

    /**
     * Get the channel parameter for the egress channel.
     *
     * @return the channel parameter for the egress channel.
     * @see Configuration#EGRESS_CHANNEL_PROP_NAME
     */
    const std::string &egressChannel() const
    {
      return m_egressChannel;
    }

    /**
     * Set the stream id for the egress channel.
     *
     * @param streamId for the egress channel.
     * @return *this for a fluent API
     * @see Configuration#EGRESS_STREAM_ID_PROP_NAME
     */
    Context &egressStreamId(std::int32_t streamId)
    {
      m_egressStreamId = streamId;
      return *this;
    }

    /**
     * Get the stream id for the egress channel.
     *
     * @return the stream id for the egress channel.
     * @see Configuration#EGRESS_STREAM_ID_PROP_NAME
     */
    std::int32_t egressStreamId() const
    {
      return m_egressStreamId;
    }

    /**
     * Set the top level Aeron directory used for communication between the Aeron client and Media Driver.
     *
     * @param aeronDirectoryName the top level Aeron directory.
     * @return *this for a fluent API.
     */
    Context &aeronDirectoryName(const std::string &aeronDirectoryName)
    {
      m_aeronDirectoryName = aeronDirectoryName;
      return *this;
    }

    /**
     * Get the top level Aeron directory used for communication between the Aeron client and Media Driver.
     *
     * @return The top level Aeron directory.
     */
    const std::string &aeronDirectoryName() const
    {
      return m_aeronDirectoryName;
    }
    
    /**
     * {@link Aeron} client for communicating with the local Media Driver.
     * <p>
     * This client will be closed when the {@link AeronCluster#close()} or {@link #close()} methods are called if
     * {@link #ownsAeronClient()} is true.
     *
     * @param aeron client for communicating with the local Media Driver.
     * @return *this for a fluent API.
     * @see Aeron#connect()
     */
    Context& aeron(std::shared_ptr<Aeron> aeron)
    {
      m_aeron = aeron;
      return *this;
    }
    /**
     * {@link Aeron} client for communicating with the local Media Driver.
     * <p>
     * If not provided then a default will be established during {@link #conclude()} by calling
     * {@link Aeron#connect()}.
     *
     * @return client for communicating with the local Media Driver.
     */
    std::shared_ptr<Aeron> aeron()
    {
      return m_aeron;
    }

    /**
     * Does this context own the {@link #aeron()} client and this takes responsibility for closing it?
     *
     * @param ownsAeronClient does this context own the {@link #aeron()} client.
     * @return *this for a fluent API.
     */
    Context& ownsAeronClient(bool ownsAeronClient)
    {
      m_ownsAeronClient = ownsAeronClient;
      return *this;
    }
    
    /**
     * Does this context own the {@link #aeron()} client and this takes responsibility for closing it?
     *
     * @return does this context own the {@link #aeron()} client and this takes responsibility for closing it?
     */
    bool ownsAeronClient() const
    {
      return m_ownsAeronClient;
    }

    /**
     * Is ingress to the cluster exclusively from a single thread to this client? The client should not be used
     * from another thread, e.g. a separate thread calling {@link AeronCluster#sendKeepAlive()} - which is awful
     * design by the way!
     *
     * @param isIngressExclusive true if ingress to the cluster is exclusively from a single thread for this client?
     * @return *this for a fluent API.
     */
    void isIngressExclusive(bool isIngressExclusive)
    {
      m_isIngressExclusive = isIngressExclusive;
    }

    void conclude();

    void close()
    {}

  private:
    /**
     * Using an integer because there is no support for boolean. 1 is concluded, 0 is not concluded.
     */
    std::int32_t m_isConcluded;

    std::int64_t m_messageTimeoutNs;
    std::string m_ingressEndpoints;
    std::string m_ingressChannel;
    std::int32_t m_ingressStreamId;
    std::string m_egressChannel;
    std::int32_t m_egressStreamId;
    std::string m_aeronDirectoryName;
    std::shared_ptr<Aeron> m_aeron;
    bool m_ownsAeronClient;
    bool m_isIngressExclusive;
    //ErrorHandler m_errorHandler;
    bool m_isDirectAssemblers;
    EgressListener m_egressListener;
    std::unique_ptr<ControlledEgressListener> m_controlledEgressListener;
    //AgentInvoker m_agentInvoker;
  };

  class AsyncConnect
  {
  public:
    AsyncConnect(Context &context, std::int64_t deadlineNs);

    /**
     * Poll to advance steps in the connection until complete or error.
     *
     * @return null if not yet complete then {@link AeronCluster} when complete.
     */
    std::shared_ptr<AeronCluster> poll();

    void close();

    /**
     * Indicates which step in the connect process has been reached.
     *
     * @return which step in the connect process has reached.
     */
    int step() const
    {
      return m_step;
    }
    
  private:

    void step(int newStep)
    {
      //System.out.println("AeronCluster.AsyncConnect " + stepName(step) + " -> " + stepName(newStep));
      m_step = newStep;
    }

    void checkDeadline();
    void createEgressSubscription();
    void createIngressPublications();
    void awaitPublicationConnected();
    void sendMessage();
    void pollResponse();
    std::shared_ptr<AeronCluster> concludeConnect();
    
    static const char* stepName(int step);

    std::int64_t m_deadlineNs;
    std::int64_t m_correlationId = NULL_VALUE;
    std::int64_t m_clusterSessionId;
    std::int64_t m_leadershipTermId;
    std::int32_t m_leaderMemberId;
    int m_step = -1;
    std::int64_t m_messageLength = 0;
    Context m_context;
    std::shared_ptr<Subscription> m_egressSubscription;
    EgressPoller m_egressPoller;
    std::int64_t m_egressRegistrationId = NULL_VALUE;
    std::map<std::int32_t, int> m_memberByIdMap;
    std::int64_t m_ingressRegistrationId = NULL_VALUE;
    std::shared_ptr<Publication> m_ingressPublication;
    
  };

  static std::shared_ptr<AeronCluster> connect()
  {
    Context context{};
    return connect(context);
  }

  static std::shared_ptr<AeronCluster> connect(Context &context);

private:
  std::int64_t m_clusterSessionId;
  std::int64_t m_leadershipTermId;
  std::int32_t m_leaderMemberId;
  bool m_isClosed;
  std::shared_ptr<Subscription> m_subscription;
  int m_egressImage;
  std::shared_ptr<Publication> m_publication;
  char m_headerBuffer[SESSION_HEADER_LENGTH];
  MessageHeader m_message;
  SessionMessageHeader m_sessionMessage;
  
  
};

}}}

#endif // AERON_CLUSTER_AERON_CLUSTER_H
