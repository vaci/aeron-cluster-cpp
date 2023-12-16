#ifndef AERON_CLUSTER_AERON_CLUSTER_H
#define AERON_CLUSTER_AERON_CLUSTER_H

#include "Aeron.h"
#include "client/AeronArchive.h"
#include "ClusterConfiguration.h"
#include "aeron_cluster_codecs/MessageHeader.h"
#include "aeron_cluster_codecs/SessionMessageHeader.h"
#include "ControlledEgressListener.h"
#include "EgressPoller.h"
#include "EgressAdapter.h"
#include "EgressListener.h"

#include <map>

namespace aeron { namespace cluster { namespace client {

class AeronCluster
{
public:
  using MessageHeader = codecs::MessageHeader;
  using SessionMessageHeader = codecs::SessionMessageHeader;

  /**
   * Length of a session message header for cluster ingress or egress.
   */
  constexpr static std::uint64_t SESSION_HEADER_LENGTH =
    MessageHeader::encodedLength() + SessionMessageHeader::sbeBlockLength();

  class Context
  {
  public:
    using this_t = Context;
    using CredentialsSupplier = archive::client::CredentialsSupplier;

    /**
     * Set the message timeout in nanoseconds to wait for sending or receiving a message.
     *
     * @param messageTimeoutNs to wait for sending or receiving a message.
     * @return this for a fluent API.
     * @see Configuration#MESSAGE_TIMEOUT_PROP_NAME
     */
    inline this_t &messageTimeoutNs(std::int64_t messageTimeoutNs)
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
    inline std::int64_t messageTimeoutNs() const
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
    inline this_t &ingressEndpoints(const std::string &clusterMembers)
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
    inline const std::string &ingressEndpoints() const
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
    inline this_t &ingressChannel(const std::string &channel)
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
    inline const std::string &ingressChannel() const
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
    inline this_t &ingressStreamId(std::int32_t streamId)
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
    inline this_t &egressChannel(const std::string &channel)
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
    inline const std::string &egressChannel() const
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
    inline this_t &egressStreamId(std::int32_t streamId)
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
    inline std::int32_t egressStreamId() const
    {
      return m_egressStreamId;
    }

    /**
     * Set the top level Aeron directory used for communication between the Aeron client and Media Driver.
     *
     * @param aeronDirectoryName the top level Aeron directory.
     * @return *this for a fluent API.
     */
    inline this_t &aeronDirectoryName(const std::string &aeronDirectoryName)
    {
      m_aeronDirectoryName = aeronDirectoryName;
      return *this;
    }

    /**
     * Get the top level Aeron directory used for communication between the Aeron client and Media Driver.
     *
     * @return The top level Aeron directory.
     */
    inline const std::string &aeronDirectoryName() const
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
    inline this_t &aeron(std::shared_ptr<Aeron> aeron)
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
    inline this_t &ownsAeronClient(bool ownsAeronClient)
    {
      m_ownsAeronClient = ownsAeronClient;
      return *this;
    }

    /**
     * Does this context own the {@link #aeron()} client and this takes responsibility for closing it?
     *
     * @return does this context own the {@link #aeron()} client and this takes responsibility for closing it?
     */
    inline bool ownsAeronClient() const
    {
      return m_ownsAeronClient;
    }

    /**
     * Get the error handler that will be called for asynchronous errors.
     *
     * @return the error handler that will be called for asynchronous errors.
     */
    inline exception_handler_t errorHandler() const
    {
      return m_errorHandler;
    }

    /**
     * Handle errors returned asynchronously from the archive for a control session.
     *
     * @param errorHandler method to handle objects of type std::exception.
     * @return this for a fluent API.
     */
    inline this_t &errorHandler(const exception_handler_t &errorHandler)
    {
      m_errorHandler = errorHandler;
      return *this;
    }

    /**
     * Is ingress to the cluster exclusively from a single thread to this client? The client should not be used
     * from another thread, e.g. a separate thread calling {@link AeronCluster#sendKeepAlive()} - which is awful
     * design by the way!
     *
     * @param isIngressExclusive true if ingress to the cluster is exclusively from a single thread for this client?
     * @return *this for a fluent API.
     */
    inline void isIngressExclusive(bool isIngressExclusive)
    {
      m_isIngressExclusive = isIngressExclusive;
    }

    inline on_new_leader_event_t newLeaderEventConsumer() const
    {
      return m_onNewLeaderEvent;
    }

    inline this_t &newLeaderEventConsumer(const on_new_leader_event_t &newLeaderEventConsumer)
    {
      m_onNewLeaderEvent = newLeaderEventConsumer;
      return *this;
    }

    inline on_session_message_t sessionMessageConsumer() const
    {
      return m_onSessionMessage;
    }

    inline this_t &sessionMessageConsumer(const on_session_message_t &sessionMessageConsumer)
    {
      m_onSessionMessage = sessionMessageConsumer;
      return *this;
    }

    inline on_session_event_t sessionEventConsumer() const
    {
      return m_onSessionEvent;
    }

    inline this_t &sessionEventConsumer(const on_session_event_t &sessionEventConsumer)
    {
      m_onSessionEvent = sessionEventConsumer;
      return *this;
    }

    inline on_admin_response_t adminResponseConsumer() const
    {
      return m_onAdminResponse;
    }

    inline this_t &adminResponseConsumer(const on_admin_response_t &adminResponseConsumer)
    {
      m_onAdminResponse = adminResponseConsumer;
      return *this;
    }

    /**
     * get the credential supplier that will be called for generating encoded credentials.
     *
     * @return the credential supplier that will be called for generating encoded credentials.
     */
    inline CredentialsSupplier &credentialsSupplier()
    {
      return m_credentialsSupplier;
    }

    /**
     * Set the CredentialSupplier functions to be called as connect requests are handled.
     *
     * @param supplier that holds functions to be called.
     * @return this for a fluent API.
     */
    inline this_t &credentialsSupplier(const CredentialsSupplier &supplier)
    {
      m_credentialsSupplier.m_encodedCredentials = supplier.m_encodedCredentials;
      m_credentialsSupplier.m_onChallenge = supplier.m_onChallenge;
      m_credentialsSupplier.m_onFree = supplier.m_onFree;
      return *this;
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
    CredentialsSupplier m_credentialsSupplier;
    bool m_isIngressExclusive;
    exception_handler_t m_errorHandler;
    bool m_isDirectAssemblers;
    on_new_leader_event_t m_onNewLeaderEvent;
    on_session_message_t m_onSessionMessage;
    on_session_event_t m_onSessionEvent;
    on_admin_response_t m_onAdminResponse;
    //AgentInvoker m_agentInvoker;
  };

  struct MemberIngress
  {
    std::int32_t m_memberId = NULL_VALUE;
    std::int64_t m_registrationId = NULL_VALUE;
    std::string m_endpoint;
    std::shared_ptr<ExclusivePublication> m_publication = nullptr;
    //std::unique_ptr<RegistrationException> m_publicationException;

    MemberIngress(std::int32_t memberId, const std::string &endpoint) :
      m_memberId(memberId),
      m_endpoint(endpoint)
    {}

    ~MemberIngress()
    {
      close();
    }

    void close()
    {
      if (m_publication != nullptr)
      {
	m_publication->close();
	m_publication = nullptr;
      }
      m_registrationId = NULL_VALUE;
    }

    std::string toString() const
    {
      return std::string(
	"MemberIngress{") +
	"memberId=" + std::to_string(m_memberId) +
	", endpoint='" + m_endpoint + '\'' +
	", publication=" + m_publication->channel() +
	'}';
    }
  };

  using MemberIngressMap = std::map<std::int32_t, MemberIngress>;

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
    void prepareConnectRequest(const std::string &channel);
    void prepareChallengeResponse(std::pair<const char*, uint32_t>);
    std::shared_ptr<AeronCluster> concludeConnect();
    void updateMembers();
    static const char* stepName(int step);

    std::int64_t m_deadlineNs;
    std::unique_ptr<Context> m_ctx;
    std::int64_t m_correlationId = NULL_VALUE;
    std::int64_t m_clusterSessionId;
    std::int64_t m_leadershipTermId;
    std::int32_t m_leaderMemberId;
    int m_step = -1;
    std::int64_t m_messageLength = 0;
    std::shared_ptr<Subscription> m_egressSubscription;
    std::unique_ptr<EgressPoller> m_egressPoller;
    std::int64_t m_egressRegistrationId = NULL_VALUE;

    AeronCluster::MemberIngressMap m_memberByIdMap;
    std::int64_t m_ingressRegistrationId = NULL_VALUE;
    std::shared_ptr<ExclusivePublication> m_ingressPublication;

    std::unique_ptr<Image> m_egressImage;

    std::uint8_t m_buffer[1024];
  };


  static std::shared_ptr<AeronCluster> connect()
  {
    Context context{};
    return connect(context);
  }

  static std::shared_ptr<AeronCluster> connect(Context &context);

  static AsyncConnect asyncConnect(Context &ctx);

  AeronCluster(
    std::unique_ptr<Context> ctx,
    std::shared_ptr<ExclusivePublication> publication,
    std::shared_ptr<Subscription> subscription,
    Image egressImage,
    MemberIngressMap memberByIdMap,
    std::int64_t clusterSessionId,
    std::int64_t leadershipTermId,
    std::int32_t leaderMemberId
  );

  inline Context &context()
  {
    return *m_ctx;
  }

private:
  std::unique_ptr<Context> m_ctx;
  std::int64_t m_clusterSessionId;
  std::int64_t m_leadershipTermId;
  std::int32_t m_leaderMemberId;
  bool m_isClosed;
  std::shared_ptr<Subscription> m_subscription;
  Image m_egressImage;
  std::shared_ptr<ExclusivePublication> m_publication;
  char m_headerBuffer[SESSION_HEADER_LENGTH];
  codecs::MessageHeader m_message;
  codecs::SessionMessageHeader m_sessionMessage;
  MemberIngressMap m_memberByIdMap;

  friend class AsyncConnect;
};

}}}

#endif // AERON_CLUSTER_AERON_CLUSTER_H
