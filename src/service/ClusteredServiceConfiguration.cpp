#include "ClusteredServiceConfiguration.h"
#include "client/ClusterException.h"

namespace aeron { namespace cluster { namespace service {

using ClusterException = client::ClusterException;

void Context::conclude()
{

  if (m_serviceId < 0 || m_serviceId > 127)
  {
    throw ClusterException(std::string("service id outside allowed range (0-127): ") + std::to_string(m_serviceId), SOURCEINFO);
  }

  if (!m_aeron)
  {
    aeron::Context ctx;

    ctx.aeronDir(m_aeronDirectoryName);
    m_aeron = Aeron::connect(ctx);
    m_ownsAeronClient = true;
  }

  if (!m_archiveContext.controlRequestChannel().find_first_of(IPC_CHANNEL) != 0)
  {
    throw ClusterException("local archive control must be IPC", SOURCEINFO);
  }
  
  if (!m_archiveContext.controlResponseChannel().find_first_of(IPC_CHANNEL) != 0)
  {
    throw ClusterException("local archive control must be IPC", SOURCEINFO);
  }

  m_archiveContext
    .aeron(m_aeron)
    .ownsAeronClient(false)
    .errorHandler(m_errorHandler);
  
  //applyDefaultParams(m_controlRequestChannel);
  //applyDefaultParams(m_controlResponseChannel);
}

}}}
