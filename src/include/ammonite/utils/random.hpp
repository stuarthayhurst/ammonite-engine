#ifndef AMMONITERANDOM
#define AMMONITERANDOM

#include <limits>
#include <random>

#include "../visibility.hpp"

namespace AMMONITE_EXPOSED ammonite {
  namespace utils {
    namespace internal {
      /*
       - Defined inside the library, exposed for usage here
       - Don't use this directly, use the helpers instead
      */
      //NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
      extern thread_local std::random_device r;
      extern thread_local std::default_random_engine engine;
      //NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)
    }

    /*
     - Return a random value of a given type from the closed interval [lower, upper]
       - Negative numbers are supported
    */
    template <typename T> requires std::is_integral_v<T> || std::is_floating_point_v<T>
    inline T random(T lower, T upper) {
      if constexpr (std::is_integral_v<T>) {
        return std::uniform_int_distribution<T>{lower, upper}(internal::engine);
      } else {
        const T upperBoundClosed = std::nextafter(upper, std::numeric_limits<T>::max());
        return std::uniform_real_distribution<T>{lower, upperBoundClosed}(internal::engine);
      }
    }

    //Return a random value of a given type from the closed interval [0, upper]
    template <typename T> requires std::is_integral_v<T> || std::is_floating_point_v<T>
    inline T random(T upper) {
      return random(T(0.0), upper);
    }

    //Return a random value of a given type from the closed interval [0, max]
    template <typename T> requires std::is_integral_v<T> || std::is_floating_point_v<T>
    inline T random() {
      return random(std::numeric_limits<T>::max());
    }

    /*
     - Return a random value of a given type from the open interval [lower, upper)
       - Negative numbers are supported
    */
    template <typename T> requires std::is_integral_v<T> || std::is_floating_point_v<T>
    inline T randomOpen(T lower, T upper) {
      if constexpr (std::is_integral_v<T>) {
        return std::uniform_int_distribution<T>{lower, upper - (T)1}(internal::engine);
      } else {
        return std::uniform_real_distribution<T>{lower, upper}(internal::engine);
      }
    }

    //Return a random value of a given type from the open interval [0, upper)
    template <typename T> requires std::is_integral_v<T> || std::is_floating_point_v<T>
    inline T randomOpen(T upper) {
      return randomOpen(T(0.0), upper);
    }

    //Return a random value of a given type from the open interval [0, max)
    template <typename T> requires std::is_integral_v<T> || std::is_floating_point_v<T>
    inline T randomOpen() {
      return randomOpen(std::numeric_limits<T>::max());
    }

    bool randomBool(double probability);
    bool randomBool();
  }
}

#endif
