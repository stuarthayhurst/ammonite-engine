#ifndef INTERNALFILES
#define INTERNALFILES

#include <cstddef>
#include <ctime>
#include <string>

#include "../enums.hpp"

namespace ammonite {
  namespace utils {
    namespace files {
      //Exported by the engine
      void deleteFile(std::string filePath);
      bool getFileMetadata(std::string filePath, std::size_t* filesize, std::time_t* timestamp);

      bool useDataCache(std::string dataCachePath);
      bool getCacheEnabled();

      unsigned char* loadFile(std::string filePath, std::size_t* size);
      bool writeFile(std::string filePath, unsigned char* data, std::size_t size);
      unsigned char* getCachedFile(std::string* cacheFilePath, std::string* filePaths,
                                   unsigned int fileCount, std::size_t* dataSize,
                                   unsigned char** userData, std::size_t* userDataSize,
                                   AmmoniteEnum* cacheState);
      bool writeCacheFile(std::string cacheFilePath, std::string* filePaths,
                          unsigned int fileCount, unsigned char* data, std::size_t dataSize,
                          unsigned char* userData, std::size_t userDataSize);
    }
  }
}

#endif
