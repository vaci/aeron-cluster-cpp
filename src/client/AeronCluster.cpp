#include "AeronCluster.h"

namespace aeron { namespace cluster { namespace client {

const char* AeronCluster::AsyncConnect::stepName(int step)
{
  switch (step)
  {
  case -1: return "CREATE_EGRESS_SUBSCRIPTION";
  case 0: return "CREATE_INGRESS_PUBLICATIONS";
  case 1: return "AWAIT_PUBLICATION_CONNECTED";
  case 2: return "SEND_MESSAGE";
  case 3: return "POLL_RESPONSE";
  case 4: return "CONCLUDE_CONNECT";
  case 5: return "DONE";
  default: return "<unknown>";
  }
}

void AeronCluster::Context::conclude()
{
  // TODO
  /*
  AtomicBuffer buffer(&m_isConcluded, sizeof(m_isConcluded));
  if (0 != buffer.getAndSetInt32(0, 1))
  {
    throw 1;
  }
  */
}

void AeronCluster::AsyncConnect::createEgressSubscription()
{
}

void AeronCluster::AsyncConnect::createIngressPublications()
{
}

void AeronCluster::AsyncConnect::awaitPublicationConnected()
{
}

void AeronCluster::AsyncConnect::sendMessage()
{
}

void AeronCluster::AsyncConnect::pollResponse()
{
}

std::shared_ptr<AeronCluster> AeronCluster::AsyncConnect::concludeConnect()
{
  return nullptr;
}

std::shared_ptr<AeronCluster> AeronCluster::AsyncConnect::poll()
{
  std::shared_ptr<AeronCluster> aeronCluster = nullptr;

  checkDeadline();

  switch (m_step)
  {
  case -1:
    createEgressSubscription();
    break;
  case 0:
    createIngressPublications();
    break;
  case 1:
    awaitPublicationConnected();
    break;
  case 2:
    sendMessage();
    break;
  case 3:
    pollResponse();
    break;
  }

  if (4 == m_step)
  {
    aeronCluster = concludeConnect();
  }

  return aeronCluster;
}

void AeronCluster::AsyncConnect::checkDeadline()
{
  if (m_deadlineNs < 0)
  {
    bool isConnected = nullptr != m_egressSubscription && m_egressSubscription->isConnected();
    
    std::string endpointPort = nullptr != m_egressSubscription
      ? m_egressSubscription->tryResolveChannelEndpointPort()
      : "<unknown>";

    // TODO
    int ex;
    /*
    TimeoutException ex(
      "cluster connect timeout: step=" + stepName(m_step) +
      " messageTimeout=" + m_context.messageTimeoutNs() + "ns" +
      " ingressChannel=" + m_context.ingressChannel() +
      " ingressEndpoints=" + m_context.ingressEndpoints() +
      " ingressPublication=" + m_ingressPublication +
      " egress.isConnected=" + m_isConnected +
      " responseChannel=" + endpointPort);
    */
    for (auto &&entry: m_memberByIdMap)
    {
      auto &member = entry.second;
      // TODO
      /*
      if (nullptr != member.m_publicationException)
      {
      }
      */
    }

    throw ex;
  }
}

  
void AeronCluster::AsyncConnect::close()
{
  if (5 != m_step)
  {
    // TODO
    //	ErrorHandler errorHandler = m_context.errorHandler();
    m_ingressPublication->close();
    m_egressSubscription->closeAndRemoveImages();
    for (auto&& entry: m_memberByIdMap)
    {
      
    }
    m_context.close();
  }  
}


std::shared_ptr<AeronCluster> AeronCluster::connect(AeronCluster::Context &context)
{
  // TODO
  return nullptr;
}

}}}
