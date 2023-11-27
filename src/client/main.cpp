#include "AeronCluster.h"

using namespace ::aeron::cluster::client;

int main(int argc, char **argv)
{
  AeronCluster::Context ctx{};
  ctx.aeronDirectoryName("./aeronDir");
  ctx.ingressChannel("aeron:ipc");
  ctx.ingressStreamId(1);
  ctx.egressChannel("aeron:ipc");
  ctx.egressStreamId(2);
  ctx.messageTimeoutNs(1000*1000*60);
  auto asyncConnect = AeronCluster::asyncConnect(ctx);

  for (auto cluster = asyncConnect.poll(); cluster == nullptr; cluster = asyncConnect.poll())
  {
  }
}
