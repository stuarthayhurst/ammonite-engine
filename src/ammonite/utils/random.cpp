#include <cmath>
#include <cstdint>
#include <limits>
#include <random>

#include "random.hpp"

namespace ammonite {
  namespace utils {
    namespace {
      //Create a random engine and seed it
      std::random_device r;
      std::default_random_engine engine(r());
    }

    /*
     - Return a random unsigned integer from the closed interval [lower, upper]
    */
    uintmax_t randomUInt(uintmax_t lower, uintmax_t upper) {
      return std::uniform_int_distribution<uintmax_t>{lower, upper}(engine);
    }

    //Return a random unsigned integer from the closed interval [0, upper]
    uintmax_t randomUInt(uintmax_t upper) {
      return std::uniform_int_distribution<uintmax_t>{0, upper}(engine);
    }

    /*
     - Return a random integer from the closed interval [lower, upper]
       - Negative numbers are supported
    */
    intmax_t randomInt(intmax_t lower, intmax_t upper) {
      return std::uniform_int_distribution<intmax_t>{lower, upper}(engine);
    }

    //Return a random integer from the closed interval [0, upper]
    intmax_t randomInt(intmax_t upper) {
      return std::uniform_int_distribution<intmax_t>{0, upper}(engine);
    }

    /*
     - Return a random double from the open interval [lower, upper)
       - Negative numbers are supported
    */
    double randomDouble(double lower, double upper) {
      return std::uniform_real_distribution<double>{lower, upper}(engine);
    }

    //Return a random double from the open interval [0, upper)
    double randomDouble(double upper) {
      return std::uniform_real_distribution<double>{0, upper}(engine);
    }

    /*
     - Return a random double from the closed interval [lower, upper]
       - Negative numbers are supported
    */
    double randomDoubleClosed(double lower, double upper) {
      return std::uniform_real_distribution<double>{lower,
        std::nextafter(upper, std::numeric_limits<double>::max())}(engine);
    }

    //Return a random double from the closed interval [0, upper]
    double randomDoubleClosed(double upper) {
      return std::uniform_real_distribution<double>{0,
        std::nextafter(upper, std::numeric_limits<double>::max())}(engine);
    }

    /*
     - Return true according to the probability passed
       - The probability is the chance of returning true
    */
    bool randomBool(double probability) {
      double result = std::uniform_real_distribution<double>{0, 1.0}(engine);
      return result < probability;
    }
  }
}
