#include "ClusterMarkFile.h"
#include "client/ClusterException.h"
#include "util/MemoryMappedFile.h"

#include <sys/stat.h>

namespace aeron { namespace cluster { namespace service {

using ClusterException = client::ClusterException;

bool ClusterMarkFile::isServiceMarkFile(const std::string &path)
{
  return path.find_first_of(SERVICE_FILENAME_PREFIX) == 0; 
  // && fileName.endsWith(FILE_EXTENSION);
}

bool ClusterMarkFile::isConsensusModuleMarkFile(const std::string &path)
{
  return path == FILENAME;
}

bool fileExists(const char *path)
{
    struct stat stat_info = {};
    return stat(path, &stat_info) == 0;
}

ClusterMarkFile::ClusterMarkFile(
  const std::string &markFilename,
  ClusterComponentType::Value type,
  std::int32_t errorBufferLength,
  EpochClock &epochClock,
  int64_t timeoutMs)
{

  if (errorBufferLength < ERROR_BUFFER_MIN_LENGTH || errorBufferLength > ERROR_BUFFER_MAX_LENGTH)
  {
    //throw new IllegalArgumentException("Invalid errorBufferLength: " + errorBufferLength);
  }
  
  int totalFileLength = HEADER_LENGTH + errorBufferLength;
  bool markFileExists = fileExists(markFilename.c_str());

  if (!markFileExists)
  {
    
  }

  m_mapFile = markFileExists
    ? util::MemoryMappedFile::mapExisting(markFilename.c_str())
    : util::MemoryMappedFile::createNew(markFilename.c_str(), 0, totalFileLength, false);

  auto buffer = m_mapFile->getMemoryPtr();
  auto length = m_mapFile->getMemorySize();

  m_header = MarkFileHeader(reinterpret_cast<char*>(buffer), length);

  auto version = m_header.version();
  if (version == VERSION_FAILED && markFileExists)
  {
  }

  AtomicBuffer errorBuffer(buffer + HEADER_LENGTH, errorBufferLength);

  if (markFileExists)
  {
    auto existingErrorBufferLength = m_header.errorBufferLength();
    AtomicBuffer existingErrorBuffer(buffer + m_header.headerLength(), existingErrorBufferLength);
    
    //saveExistingErrors(file, existingErrorBuffer, type, CommonContext.fallbackLogger());
    existingErrorBuffer.setMemory(0, existingErrorBufferLength, 0);
  }
  else
  {
    m_header.candidateTermId(NULL_VALUE);
  }

  auto existingType = m_header.componentType();

  if (existingType != ClusterComponentType::Value::NULL_COMPONENT && existingType != type)
  {
    if (existingType != ClusterComponentType::BACKUP ||
	ClusterComponentType::CONSENSUS_MODULE != type)
    {
      throw new ClusterException(
	std::string("existing Mark file type ") + ClusterComponentType::c_str(existingType) +
	" not same as required type " + ClusterComponentType::c_str(type), SOURCEINFO);
    }
  }

  m_header.componentType(type);
  m_header.headerLength(HEADER_LENGTH);
  m_header.errorBufferLength(errorBufferLength);
  m_header.pid(::getpid());
  m_header.startTimestamp(epochClock.time());
}


}}}
