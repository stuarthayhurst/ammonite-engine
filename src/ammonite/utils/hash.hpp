#ifndef INTERNALHASH
#define INTERNALHASH

#include <string>

#include "../internal.hpp"

namespace ammonite {
  namespace utils {
    namespace AMMONITE_INTERNAL internal {
      std::string hashStrings(std::string* filePaths, unsigned int fileCount);
    }
  }
}

#endif
