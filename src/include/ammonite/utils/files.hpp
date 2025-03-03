#ifndef FILES
#define FILES

#include <cstddef>
#include <ctime>
#include <string>

#include "../enums.hpp"

namespace ammonite {
  namespace utils {
    namespace files {
      void deleteFile(const std::string& filePath);
      bool getFileMetadata(const std::string& filePath, std::size_t* filesize, std::time_t* timestamp);

      bool useDataCache(const std::string& targetCachePath);
      bool getCacheEnabled();

      unsigned char* loadFile(const std::string& filePath, std::size_t* size);
      bool writeFile(const std::string& filePath, unsigned char* data, std::size_t size);
      unsigned char* getCachedFile(std::string* cacheFilePath, std::string* filePaths,
                                   unsigned int fileCount, std::size_t* dataSize,
                                   unsigned char** userData, std::size_t* userDataSize,
                                   AmmoniteEnum* cacheState);
      bool writeCacheFile(const std::string& cacheFilePath, std::string* filePaths,
                          unsigned int fileCount, unsigned char* data, std::size_t dataSize,
                          unsigned char* userData, std::size_t userDataSize);
    }
  }
}

#endif
