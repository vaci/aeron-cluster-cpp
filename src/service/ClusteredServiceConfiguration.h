#ifndef AERON_CLUSTER_CLUSTERED_SERVICE_CONFIGURATION_H
#define AERON_CLUSTER_CLUSTERED_SERVICE_CONFIGURATION_H

#include <string>
#include "Aeron.h"
#include "client/ArchiveConfiguration.h"
#include "ClusteredService.h"
#include "aeron/aeron_counters.h"

namespace aeron { namespace cluster { namespace service {

namespace Configuration
{
/**
 * Type of snapshot for this service.
 */
constexpr std::int64_t SNAPSHOT_TYPE_ID = 2;

/**
 * Update interval for cluster mark file in nanoseconds.
 */
constexpr std::int64_t MARK_FILE_UPDATE_INTERVAL_NS = 1000;

/**
 * Timeout in milliseconds to detect liveness.
 */
constexpr std::int64_t LIVENESS_TIMEOUT_MS = 10 * 1000;

/**
 * Property name for the identity of the cluster instance.
 */
constexpr const char *CLUSTER_ID_PROP_NAME = "aeron.cluster.id";

/**
 * Default identity for a clustered instance.
 */
constexpr std::int32_t CLUSTER_ID_DEFAULT = 0;

/**
 * Identity for a clustered service. Services should be numbered from 0 and be contiguous.
 */
constexpr const char *SERVICE_ID_PROP_NAME = "aeron.cluster.service.id";

/**
 * Default identity for a clustered service.
 */
constexpr std::int32_t SERVICE_ID_DEFAULT = 0;

/**
 * Name for a clustered service to be the role of the {@link Agent}.
 */
constexpr const char *SERVICE_NAME_PROP_NAME = "aeron.cluster.service.name";

/**
 * Name for a clustered service to be the role of the {@link Agent}.
 */
constexpr const char *SERVICE_NAME_DEFAULT = "clustered-service";

/**
 * Class name for dynamically loading a {@link ClusteredService}. This is used if
 * {@link Context#clusteredService()} is not set.
 */
constexpr const char *SERVICE_CLASS_NAME_PROP_NAME = "aeron.cluster.service.class.name";

/**
 * Channel to be used for log or snapshot replay on startup.
 */
constexpr const char *REPLAY_CHANNEL_PROP_NAME = "aeron.cluster.replay.channel";

/**
 * Default channel to be used for log or snapshot replay on startup.
 */
constexpr const char *REPLAY_CHANNEL_DEFAULT = "ipc";

/**
 * Stream id within a channel for the clustered log or snapshot replay.
 */
constexpr const char *REPLAY_STREAM_ID_PROP_NAME = "aeron.cluster.replay.stream.id";

/**
 * Default stream id for the log or snapshot replay within a channel.
 */
constexpr std::int32_t REPLAY_STREAM_ID_DEFAULT = 103;

/**
 * Channel for control communications between the local consensus module and services.
 */
constexpr const char *CONTROL_CHANNEL_PROP_NAME = "aeron.cluster.control.channel";

/**
 * Default channel for communications between the local consensus module and services. This should be IPC.
 */
constexpr const char *CONTROL_CHANNEL_DEFAULT = "aeron:ipc?term-length=128k";

/**
 * Stream id within the control channel for communications from the consensus module to the services.
 */
constexpr const char *SERVICE_STREAM_ID_PROP_NAME = "aeron.cluster.service.stream.id";

/**
 * Default stream id within the control channel for communications from the consensus module.
 */
constexpr std::int32_t SERVICE_STREAM_ID_DEFAULT = 104;

/**
 * Stream id within the control channel for communications from the services to the consensus module.
 */
constexpr const char *CONSENSUS_MODULE_STREAM_ID_PROP_NAME = "aeron.cluster.consensus.module.stream.id";

/**
 * Default stream id within a channel for communications from the services to the consensus module.
 */
constexpr std::int32_t CONSENSUS_MODULE_STREAM_ID_DEFAULT = 105;

/**
 * Channel to be used for archiving snapshots.
 */
constexpr const char *SNAPSHOT_CHANNEL_PROP_NAME = "aeron.cluster.snapshot.channel";

/**
 * Default channel to be used for archiving snapshots.
 */
constexpr const char *SNAPSHOT_CHANNEL_DEFAULT = "aeron:ipc?alias=snapshot";

/**
 * Stream id within a channel for archiving snapshots.
 */
constexpr const char *SNAPSHOT_STREAM_ID_PROP_NAME = "aeron.cluster.snapshot.stream.id";

/**
 * Default stream id for the archived snapshots within a channel.
 */
constexpr std::int32_t SNAPSHOT_STREAM_ID_DEFAULT = 106;

/**
 * Directory to use for the aeron cluster.
 */
constexpr const char *CLUSTER_DIR_PROP_NAME = "aeron.cluster.dir";

/**
 * Directory to use for the Cluster component's mark file.
 */
constexpr const char *MARK_FILE_DIR_PROP_NAME = "aeron.cluster.mark.file.dir";

/**
 * Default directory to use for the aeron cluster.
 */
constexpr const char *CLUSTER_DIR_DEFAULT = "aeron-cluster";

/**
 * Length in bytes of the error buffer for the cluster container.
 */
constexpr const char *ERROR_BUFFER_LENGTH_PROP_NAME = "aeron.cluster.service.error.buffer.length";

/**
 * Default length in bytes of the error buffer for the cluster container.
 */
constexpr std::int32_t ERROR_BUFFER_LENGTH_DEFAULT = 1024 * 1024;

/**
 * Is this a responding service to client requests property.
 */
constexpr const char *RESPONDER_SERVICE_PROP_NAME = "aeron.cluster.service.responder";

/**
 * Default to true that this a responding service to client requests.
 */
constexpr bool RESPONDER_SERVICE_DEFAULT = true;

/**
 * Fragment limit to use when polling the log.
 */
constexpr const char *LOG_FRAGMENT_LIMIT_PROP_NAME = "aeron.cluster.log.fragment.limit";

/**
 * Default fragment limit for polling log.
 */
constexpr std::int32_t LOG_FRAGMENT_LIMIT_DEFAULT = 50;

/**
 * Property name for threshold value for the container work cycle threshold to track
 * for being exceeded.
 */
constexpr const char *CYCLE_THRESHOLD_PROP_NAME = "aeron.cluster.service.cycle.threshold";

/**
 * Default threshold value for the container work cycle threshold to track for being exceeded.
 */
constexpr std::int64_t CYCLE_THRESHOLD_DEFAULT_NS = 1000000;

/**
 * Counter type id for the cluster node role.
 */
constexpr std::int32_t CLUSTER_NODE_ROLE_TYPE_ID = AERON_COUNTER_CLUSTER_NODE_ROLE_TYPE_ID;

/**
 * Counter type id of the commit position.
 */
constexpr std::int32_t COMMIT_POSITION_TYPE_ID = AERON_COUNTER_CLUSTER_COMMIT_POSITION_TYPE_ID;

/**
 * Counter type id for the clustered service error count.
 */
constexpr std::int32_t CLUSTERED_SERVICE_ERROR_COUNT_TYPE_ID =
  AERON_COUNTER_CLUSTER_CLUSTERED_SERVICE_ERROR_COUNT_TYPE_ID;

  /**
   * The value {@link #SERVICE_STREAM_ID_DEFAULT} or system property
   * {@link #SERVICE_STREAM_ID_PROP_NAME} if set.
   *
   * @return {@link #SERVICE_STREAM_ID_DEFAULT} or system property
   * {@link #SERVICE_STREAM_ID_PROP_NAME} if set.
   */
  static std::int32_t serviceStreamId()
  {
    return 0;
    //return Integer.getInteger(SERVICE_STREAM_ID_PROP_NAME, SERVICE_STREAM_ID_DEFAULT);
  }
  
  /**
   * The value {@link #SNAPSHOT_CHANNEL_DEFAULT} or system property {@link #SNAPSHOT_CHANNEL_PROP_NAME} if set.
   *
   * @return {@link #SNAPSHOT_CHANNEL_DEFAULT} or system property {@link #SNAPSHOT_CHANNEL_PROP_NAME} if set.
   */
  static std::string snapshotChannel()
  {
    return "";
    //return System.getProperty(SNAPSHOT_CHANNEL_PROP_NAME, SNAPSHOT_CHANNEL_DEFAULT);
  }

  /**
   * The value {@link #SNAPSHOT_STREAM_ID_DEFAULT} or system property {@link #SNAPSHOT_STREAM_ID_PROP_NAME}
   * if set.
   *
   * @return {@link #SNAPSHOT_STREAM_ID_DEFAULT} or system property {@link #SNAPSHOT_STREAM_ID_PROP_NAME} if set.
   */
  static std::int32_t snapshotStreamId()
  {
    return 0;
    //return Integer.getInteger(SNAPSHOT_STREAM_ID_PROP_NAME, SNAPSHOT_STREAM_ID_DEFAULT);
  }

  /**
   * Property to configure if this node should take standby snapshots. The default for this property is
   * <code>false</code>.
   */
  static constexpr const char *STANDBY_SNAPSHOT_ENABLED_PROP_NAME = "aeron.cluster.standby.snapshot.enabled";

  
}

class Context
{
public:
  using this_t = Context;
  Context() {}

  inline std::shared_ptr<Aeron> aeron()
  {
    return m_aeron;
  }

  inline this_t &aeron(std::shared_ptr<Aeron> aeron)
  {
    m_aeron = aeron;
    return *this;
  }

  inline const std::string &aeronDirectoryName() const
  {
    return m_aeronDirectoryName;
  }

  inline this_t &aeronDirectoryName(const std::string &aeronDirectoryName)
  {
    m_aeronDirectoryName = aeronDirectoryName;
    return *this;
  }

  /**
   * Does this context own the Aeron client and thus takes responsibility for closing it?
   *
   * @return does this context own the Aeron client and thus takes responsibility for closing it?
   */
  inline bool ownsAeronClient() const
  {
    return m_ownsAeronClient;
  }

  /**
   * Does this context own the Aeron client and thus takes responsibility for closing it?
   *
   * @param ownsAeronClient does this context own the Aeron client?
   * @return this for a fluent API.
   */
  inline this_t &ownsAeronClient(bool ownsAeronClient)
  {
    m_ownsAeronClient = ownsAeronClient;
    return *this;
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


  inline archive::client::Context &archiveContext()
  {
    return m_archiveContext;
  }

  inline this_t &archiveContext(archive::client::Context &archiveContext)
  {
    m_archiveContext = archiveContext;
    return *this;
  }

  inline const std::string &clusterDirectoryName() const
  {
    return m_clusterDirectoryName;
  }

  inline this_t &clusterDirectoryName(const std::string &clusterDirectoryName)
  {
    m_clusterDirectoryName = clusterDirectoryName;
    return *this;
  }

  inline std::int32_t serviceId() const
  {
    return m_serviceId;
  }

  inline this_t &serviceId(std::int32_t serviceId)
  {
    m_serviceId = serviceId;
    return *this;
  }

  inline const std::string &serviceName() const
  {
    return m_serviceName;
  }

  inline this_t &serviceName(const std::string &serviceName)
  {
    m_serviceName = serviceName;
    return *this;
  }

  inline const std::string &controlChannel() const
  {
    return m_controlChannel;
  }

  inline this_t &controlChannel(const std::string &controlChannel)
  {
    m_controlChannel = controlChannel;
    return *this;
  }

  inline const std::string &replayChannel() const
  {
    return m_replayChannel;
  }

  inline this_t &replayChannel(const std::string &replayChannel)
  {
    m_replayChannel = replayChannel;
    return *this;
  }

  inline std::int32_t replayStreamId() const
  {
    return m_replayStreamId;
  }

  inline this_t &replayStreamId(std::int32_t streamId)
  {
    m_replayStreamId = streamId;
    return *this;
  }

  inline const std::string &snapshotChannel() const
  {
    return m_snapshotChannel;
  }

  inline this_t &snapshotChannel(const std::string &snapshotChannel)
  {
    m_snapshotChannel = snapshotChannel;
    return *this;
  }

  inline std::int32_t snapshotStreamId() const
  {
    return m_snapshotStreamId;
  }

  inline this_t &snapshotStreamId(std::int32_t streamId)
  {
    m_snapshotStreamId = streamId;
    return *this;
  }

  inline std::int32_t consensusModuleStreamId() const
  {
    return m_consensusModuleStreamId;
  }

  inline this_t &consensusModuleStreamId(std::int32_t streamId)
  {
    m_consensusModuleStreamId = streamId;
    return *this;
  }

  inline std::int32_t serviceStreamId() const
  {
    return m_serviceStreamId;
  }

  inline this_t &serviceStreamId(std::int32_t streamId)
  {
    m_serviceStreamId = streamId;
    return *this;
  }

  inline std::shared_ptr<ClusteredService> clusteredService()
  {
    return m_clusteredService;
  }

  inline  this_t &clusteredService(std::shared_ptr<ClusteredService> clusteredService)
  {
    m_clusteredService = clusteredService;
    return *this;
  }

  void conclude();

  /**
   * User assigned application version which appended to the log as the appVersion in new leadership events.
   * <p>
   * This can be validated using {@link org.agrona.SemanticVersion} to ensure only application nodes of the same
   * major version communicate with each other.
   *
   * @param appVersion for user application.
   * @return this for a fluent API.
   */
  inline this_t &appVersion(std::int32_t appVersion)
  {
    m_appVersion = appVersion;
    return *this;
  }

  /**
   * User assigned application version which appended to the log as the appVersion in new leadership events.
   * <p>
   * This can be validated using {@link org.agrona.SemanticVersion} to ensure only application nodes of the same
   * major version communicate with each other.
   *
   * @return appVersion for user application.
   */
  inline std::int32_t appVersion()
  {
    return m_appVersion;
  }

  /**
   * Set the id for this cluster instance. This must match with the Consensus Module.
   *
   * @param clusterId for this clustered instance.
   * @return this for a fluent API
   * @see Configuration#CLUSTER_ID_PROP_NAME
   */
  inline this_t &clusterId(std::int32_t clusterId)
  {
    m_clusterId = clusterId;
    return *this;
  }

  /**
   * Get the id for this cluster instance. This must match with the Consensus Module.
   *
   * @return the id for this cluster instance.
   * @see Configuration#CLUSTER_ID_PROP_NAME
   */
  inline std::int32_t clusterId()
  {
    return m_clusterId;
  }

  inline bool isRespondingService() const
  {
    return m_isRespondingService;
  }

  
  inline this_t &isRespondingService(bool isRespondingService)
  {
    m_isRespondingService = isRespondingService;
    return *this;
  }
  
private:
  archive::client::Context m_archiveContext;
  std::shared_ptr<Aeron> m_aeron;
  std::string m_aeronDirectoryName = aeron::Context::defaultAeronPath();
  std::string m_clusterDirectoryName;
  std::int32_t m_serviceId;
  std::string m_serviceName;
  std::string m_controlChannel = Configuration::CONTROL_CHANNEL_DEFAULT;
  std::string m_replayChannel = Configuration::REPLAY_CHANNEL_DEFAULT;
  std::int32_t m_replayStreamId;
  std::string m_snapshotChannel = Configuration::SNAPSHOT_CHANNEL_DEFAULT;
  std::int32_t m_snapshotStreamId;
  std::int32_t m_consensusModuleStreamId;
  std::int32_t m_serviceStreamId;
  std::shared_ptr<ClusteredService> m_clusteredService;
  bool m_ownsAeronClient = false;
  exception_handler_t m_errorHandler = nullptr;
  std::int32_t m_appVersion;
  std::int32_t m_clusterId;
  bool m_isRespondingService;

  inline void applyDefaultParams(std::string &channel) const
  {
    std::shared_ptr<ChannelUri> uri = ChannelUri::parse(channel);
    channel = uri->toString();
  }
};


}}}

#endif
