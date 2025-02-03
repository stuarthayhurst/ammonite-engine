#ifndef HASH
#define HASH

#include <string>

#include "../internal.hpp"

namespace ammonite {
  namespace utils {
    namespace files {
      namespace AMMONITE_INTERNAL internal {
        std::string generateCacheString(std::string* filePaths, unsigned int fileCount);
      }
    }
  }
}

#endif
