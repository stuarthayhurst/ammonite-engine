#include <random>

#include "random.hpp"

namespace ammonite {
  namespace utils {
    namespace internal {
      //Create a random engine and seed it
      //NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
      thread_local std::random_device r;
      thread_local std::default_random_engine engine(r());
      //NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)
    }

    /*
     - Return true according to the probability passed
       - The probability is the chance of returning true
    */
    bool randomBool(double probability) {
      const double result = std::uniform_real_distribution<double>{0, 1.0}(internal::engine);
      return result < probability;
    }

    //Return true 50% of the time
    bool randomBool() {
      return randomBool(0.5);
    }
  }
}
