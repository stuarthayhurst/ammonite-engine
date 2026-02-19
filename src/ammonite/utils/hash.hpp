#ifndef INTERNALHASH
#define INTERNALHASH

#include <string>

#include "../visibility.hpp"

namespace ammonite {
  namespace utils {
    namespace AMMONITE_INTERNAL internal {
      std::string hashStrings(const std::string* inputs, unsigned int inputCount);
    }
  }
}

#endif
