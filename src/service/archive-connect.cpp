#include "client/AeronArchive.h"
using IdleStrategy = ::aeron::concurrent::SleepingIdleStrategy;


using namespace aeron::archive::client;

int main(int argc, char **argv)
{
  Context ctx;
  ctx.controlRequestChannel("aeron:ipc");
  ctx.controlResponseChannel("aeron:ipc");
  
  auto asyncConnect = AeronArchive::asyncConnect(ctx);

  IdleStrategy idle(std::chrono::milliseconds(1000));
  for (auto cluster = asyncConnect->poll(); cluster == nullptr; cluster = asyncConnect->poll())
  {
    //std::cout << "Polling archive connect: " << std::to_string(asyncConnect->step()) << std::endl;
    idle.idle(0);
    std::cout << "Idling..." << std::endl;
    if (!ctx.aeron()->usesAgentInvoker())
    {
      std::cout << "Invoking..." << std::endl;
      ctx.aeron()->conductorAgentInvoker().invoke();
    }
  }

  std::cout << "Connected" << std::endl;
}
