#ifndef AMMONITEANGLE
#define AMMONITEANGLE

#include <numbers>
#include <type_traits>

namespace ammonite {
  inline namespace maths {
    template <typename T> requires std::is_floating_point_v<T>
    constexpr T radians(T angle) {
      return (angle / (T)180.0) * std::numbers::pi_v<T>;
    }

    template <typename T> requires std::is_floating_point_v<T>
    constexpr T degrees(T angle) {
      return (angle / std::numbers::pi_v<T>) * (T)180.0;
    }
  }
}

#endif
