#include <cstdint>
#include <random>

namespace ammonite {
  namespace utils {
    namespace {
      //Create a random engine and seed it
      std::random_device r;
      std::default_random_engine engine(r());
    }

    //Return a random integer from the interval [lower, upper]
    uintmax_t randomInt(uintmax_t lower, uintmax_t upper) {
      return std::uniform_int_distribution<uintmax_t>{lower, upper}(engine);
    }

    //Return a random integer from the interval [0, upper]
    uintmax_t randomInt(uintmax_t upper) {
      return std::uniform_int_distribution<uintmax_t>{0, upper}(engine);
    }

    //Return a random double from the interval [lower, upper)
    double randomDouble(double lower, double upper) {
      return std::uniform_real_distribution<double>{lower, upper}(engine);
    }

    //Return a random double from the interval [0, upper)
    double randomDouble(double upper) {
      return std::uniform_real_distribution<double>{0, upper}(engine);;
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
