#ifndef INTERNALAMMONITEANGLE
#define INTERNALAMMONITEANGLE

#include <cmath>
#include <type_traits>

//Include public interface
#include "../../include/ammonite/maths/angle.hpp" // IWYU pragma: export

#include "vector.hpp"

#include "../visibility.hpp"

namespace AMMONITE_INTERNAL ammonite {
  inline namespace maths {
    template <typename T> requires std::is_floating_point_v<T> && validVector<T, 3>
    inline ammonite::Vec<T, 3>& calculateDirection(T horizontal, T vertical,
                                                   ammonite::Vec<T, 3>& dest) {
      const ammonite::Vec<T, 3> direction = {
        std::cos(vertical) * std::sin(horizontal),
        std::sin(vertical),
        std::cos(vertical) * std::cos(horizontal)
      };

      return ammonite::normalise(direction, dest);
    }

    template <typename T> requires std::is_floating_point_v<T> && validVector<T, 3>
    inline T calculateVerticalAngle(const ammonite::Vec<T, 3>& direction) {
      ammonite::Vec<T, 3> normalisedDirection = {0};
      ammonite::normalise(direction, normalisedDirection);
      return std::asin(normalisedDirection[1]);
    }

    template <typename T> requires std::is_floating_point_v<T> && validVector<T, 3>
    inline T calculateHorizontalAngle(const ammonite::Vec<T, 3>& direction) {
      ammonite::Vec<T, 3> normalisedDirection = {0};
      ammonite::copy(direction, normalisedDirection);
      normalisedDirection[1] = (T)0.0;

      ammonite::normalise(normalisedDirection);
      return std::atan2(normalisedDirection[0], normalisedDirection[2]);
    }
  }
}

#endif
