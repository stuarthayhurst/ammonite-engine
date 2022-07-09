#ifndef CACHE
#define CACHE

#include <string>

namespace ammonite {
  namespace utils {
    namespace cache {
      std::string requestNewCache(std::string cacheString);
      std::string requestCachedData(const char* filePaths[], const int fileCount, std::string cacheString, bool* found);

      bool useDataCache(const char* dataCachePath);
      bool getCacheEnabled();
    }
  }
}

#endif
