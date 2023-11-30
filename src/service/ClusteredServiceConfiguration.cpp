#include "ClusteredServiceConfiguration.h"

namespace aeron { namespace cluster { namespace service {

void Context::conclude()
{
  if (!m_aeron)
  {
    aeron::Context ctx;

    ctx.aeronDir(m_aeronDirectoryName);
    m_aeron = Aeron::connect(ctx);
    m_ownsAeronClient = true;
  }

  //applyDefaultParams(m_controlRequestChannel);
  //applyDefaultParams(m_controlResponseChannel);
}

}}}
