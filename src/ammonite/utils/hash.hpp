#ifndef INTERNALHASH
#define INTERNALHASH

#include <string>

#include "../visibility.hpp"

namespace AMMONITE_INTERNAL ammonite {
  namespace utils {
    namespace internal {
      std::string hashStrings(const std::string* inputs, unsigned int inputCount);
    }
  }
}

#endif
