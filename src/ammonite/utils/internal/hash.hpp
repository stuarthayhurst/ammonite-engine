#ifndef HASH
#define HASH

namespace ammonite {
  namespace utils {
    namespace files {
      namespace internal {
        std::string generateCacheString(std::string* filePaths, unsigned int fileCount);
      }
    }
  }
}

#endif
