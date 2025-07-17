#ifndef AMMONITEQUATERNION
#define AMMONITEQUATERNION

#include <algorithm>
#include <cstring>
#include <string>
#include <type_traits>

//TODO: Convert to <simd> with C++26, drop header filter regex
//NOLINTNEXTLINE(misc-include-cleaner)
#include <experimental/simd>

#include "quaternionTypes.hpp"

namespace ammonite {
  //Maths operations
  inline namespace maths {
    //Copy from src to dest
    template <typename T> requires validQuaternion<T>
    constexpr void copy(const Quat<T>& src, Quat<T>& dest) {
      if consteval {
        //Slower, constexpr-friendly copy
        std::copy(&src[0][0], &src[0][4], &dest[0][0]);
      } else {
        std::memcpy(&dest[0], &src[0], sizeof(Quat<T>));
      }
    }

    /*
     - Copy from src to dest
     - Additionally, cast each element during the copy
    */
    template <typename T, typename S> requires validQuaternion<T> && validQuaternion<S>
    constexpr void copyCast(const Quat<T>& src, Quat<S>& dest) {
      if constexpr (std::is_same_v<T, S>) {
        //Faster runtime copies for equal types
        ammonite::copy(src, dest);
      } else {
        std::copy(&src[0][0], &src[0][4], &dest[0][0]);
      }
    }
  }

  //Utility / support functions
  inline namespace maths {
    template<typename T> requires validQuaternion<T>
    std::string formatQuaternion(const Quat<T>& quaternion) {
      std::string result;
      for (unsigned int i = 0; i < 4; i++) {
        if (i != 0) {
          result += ", ";
        }

        result += std::to_string(quaternion[0][i]);
      }

      return result;
    }
  }
}

#endif
