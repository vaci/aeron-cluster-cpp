#include <Aeron.h>
#include "ClusteredServiceAgent.h"
#include "ClusteredServiceConfiguration.h"

using namespace ::aeron::cluster::service;
using IdleStrategy = ::aeron::concurrent::SleepingIdleStrategy;

struct TestService
  : ClusteredService
{
  bool onStart(Cluster &cluster, std::shared_ptr<aeron::Image> snapshotImage) override
  {
    return true;
  }

  void onSessionMessage(
    ClientSession &session,
    std::int64_t timestamp,
    aeron::AtomicBuffer &buffer,
    aeron::index_t offset,
    aeron::index_t length,
    aeron::Header header) override
  {
  }

  void onSessionOpen(ClientSession &session, std::int64_t timestamp) override
  {
    std::cout << "onSessionOpen: " << timestamp << std::endl;
  }
  void onSessionClose(ClientSession &session, std::int64_t timestamp, CloseReason closeReason) override
  {
    std::cout << "onSessionClose: " << timestamp << std::endl;
  }

  void onTimerEvent(std::int64_t correlationId, std::int64_t timestamp)
  {
    std::cout << "onTimerEvent: " << timestamp << std::endl;
  }

  bool onTakeSnapshot(std::shared_ptr<aeron::ExclusivePublication> snapshotPublication)
  {
    std::cout << "onTakeSnapshot" << std::endl;
    return true;
  }

  
  void onRoleChange(Cluster::Role newRole) override
  {
    std::cout << "onRoleChange" << std::endl;
  }

  void onTerminate(Cluster &cluster) override
  {
    std::cout << "onTerminate" << std::endl;
  }    
};

int main(int argc, char **argv)
{
  Context ctx{};
  ctx.clusterDirectoryName("./cluster-1");
  ctx.serviceId(0);
  ctx.serviceName("service-1");
  //ctx.serviceStreamId(104);
  ctx.archiveContext().controlRequestChannel("aeron:ipc");
  ctx.archiveContext().controlResponseChannel("aeron:ipc");
  ctx.clusteredService(std::make_shared<TestService>());
  
  auto asyncConnect = ClusteredServiceAgent::asyncConnect(ctx);
  auto aeron = ctx.aeron();
  if (aeron != nullptr)
  {
    std::cout << "Aeron is not null" << std::endl;
  }
  IdleStrategy idle(std::chrono::milliseconds(1000));

  auto cluster = asyncConnect->poll();
  while (cluster == nullptr)
  {
    idle.idle(0);
    if (ctx.aeron()->usesAgentInvoker())
    {
      std::cout << "Invoking..." << std::endl;
      ctx.aeron()->conductorAgentInvoker().invoke();
    }
    cluster = asyncConnect->poll();
  }

  while (true)
  {
    idle.idle(0);
    cluster->doWork();
  }
}
