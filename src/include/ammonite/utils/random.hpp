#ifndef AMMONITERANDOM
#define AMMONITERANDOM

#include <cstdint>

#include "../visibility.hpp"

namespace AMMONITE_EXPOSED ammonite {
  namespace utils {
    uintmax_t randomUInt(uintmax_t lower, uintmax_t upper);
    uintmax_t randomUInt(uintmax_t upper);
    intmax_t randomInt(intmax_t lower, intmax_t upper);
    intmax_t randomInt(intmax_t upper);

    double randomDouble(double lower, double upper);
    double randomDouble(double upper);
    double randomDoubleClosed(double lower, double upper);
    double randomDoubleClosed(double upper);

    bool randomBool(double probability);
  }
}

#endif
