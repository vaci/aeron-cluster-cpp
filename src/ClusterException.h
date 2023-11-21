#ifndef AERON_CLUSTER_CLUSTER_EXCEPTION_H
#define AERON_CLUSTER_CLUSTER_EXCEPTION_H

#include "Aeron.h"

namespace aeron { namespace cluster { namespace client {

class ClusterException : public SourcedException
{
public:
  ClusterException(
    const std::string &what,
    const std::string &function,
    const std::string &file,
    const int line) :
    SourcedException(what, function, file, line)
  {
  }
};

}}}
#endif
