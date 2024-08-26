#ifndef RANDOM
#define RANDOM

#include <cstdint>

namespace ammonite {
  namespace utils {
    uintmax_t randomInt(uintmax_t lower, uintmax_t upper);
    uintmax_t randomInt(uintmax_t upper);

    double randomDouble(double lower, double upper);
    double randomDouble(double upper);
    double randomDoubleClosed(double lower, double upper);
    double randomDoubleClosed(double upper);

    bool randomBool(double probability);
  }
}

#endif
