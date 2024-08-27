#include <cmath>
#include <cstdint>
#include <limits>
#include <random>

namespace ammonite {
  namespace utils {
    namespace {
      //Create a random engine and seed it
      std::random_device r;
      std::default_random_engine engine(r());
    }

    /*
     - Return a random integer from the closed interval [lower, upper]
       - Negative numbers can be returned if returning to a signed type
       - For example:
         - (int)randomInt(-1, 1) can return the closed interval [-1, 1]
         - (uintmax_t)randomInt(-1, 1) can return UINTMAX_MAX, 0 or 1
    */
    uintmax_t randomInt(uintmax_t lower, uintmax_t upper) {
      return std::uniform_int_distribution<uintmax_t>{lower, upper}(engine);
    }

    //Return a random integer from the closed interval [0, upper]
    uintmax_t randomInt(uintmax_t upper) {
      return std::uniform_int_distribution<uintmax_t>{0, upper}(engine);
    }

    /*
     - Return a random double from the open interval [lower, upper)
       - Negative numbers are supported, without any casts
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
       - Negative numbers are supported, without any casts
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
