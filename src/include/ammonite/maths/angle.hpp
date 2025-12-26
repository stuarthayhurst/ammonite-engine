#ifndef AMMONITEANGLE
#define AMMONITEANGLE

#include <numbers>
#include <type_traits>

#include "../visibility.hpp"

namespace AMMONITE_INTERNAL ammonite {
  inline namespace maths {
    template <typename T> requires std::is_floating_point_v<T>
    constexpr T pi() {
      return std::numbers::pi_v<T>;
    }

    template <typename T> requires std::is_floating_point_v<T>
    constexpr T radians(T angle) {
      return (angle / (T)180.0) * ammonite::pi<T>();
    }

    template <typename T> requires std::is_floating_point_v<T>
    constexpr T degrees(T angle) {
      return (angle / ammonite::pi<T>()) * (T)180.0;
    }

    template <typename T> requires std::is_floating_point_v<T>
    constexpr T smallestAngleDelta(T angleA, T angleB) {
      T angleDelta = angleA - angleB;
      if (angleDelta < -ammonite::pi<T>()) {
        angleDelta += ammonite::pi<T>() * (T)2.0;
      } else if (angleDelta > ammonite::pi<T>()) {
        angleDelta -= ammonite::pi<T>() * (T)2.0;
      }

      return angleDelta;
    }
  }
}

#endif
