#ifndef AMMONITEFILES
#define AMMONITEFILES

#include <cstddef>
#include <ctime>
#include <string>

#include "../exposed.hpp"

//Cache return values / states
enum AmmoniteCacheEnum : unsigned char {
  AMMONITE_CACHE_HIT,
  AMMONITE_CACHE_MISS,
  AMMONITE_CACHE_INVALID,
  AMMONITE_CACHE_COLLISION
};

namespace AMMONITE_EXPOSED ammonite {
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
                                   AmmoniteCacheEnum* cacheState);
      bool writeCacheFile(const std::string& cacheFilePath, std::string* filePaths,
                          unsigned int fileCount, unsigned char* data, std::size_t dataSize,
                          unsigned char* userData, std::size_t userDataSize);
    }
  }
}

#endif
