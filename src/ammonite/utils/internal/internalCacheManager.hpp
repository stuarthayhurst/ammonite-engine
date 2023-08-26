#ifndef INTERNALCACHE
#define INTERNALCACHE

/* Internally exposed header:
 - Expose cache string requesting internally
*/

#include <string>

namespace ammonite {
  namespace utils {
    namespace cache {
      namespace internal {
        std::string requestNewCachePath(const char* filePaths[], const int fileCount);
        std::string requestCachedDataPath(const char* filePaths[], const int fileCount, bool* found);
      }
    }
  }
}

#endif
