#ifndef FILEMANAGER
#define FILEMANAGER

#include <cstddef>
#include <string>

#include "../enums.hpp"
#include "../types.hpp"

namespace ammonite {
  namespace files {
    namespace internal {
      void deleteFile(std::string filePath);
      bool getFileMetadata(std::string filePath, long long int* filesize, long long int* timestamp);

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
