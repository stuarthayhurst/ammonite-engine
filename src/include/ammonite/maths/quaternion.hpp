#ifndef AMMONITEQUATERNION
#define AMMONITEQUATERNION

#include <algorithm>
#include <cstring>
#include <functional>
#include <string>
#include <type_traits>

//TODO: Convert to <simd> with C++26, drop header filter regex
//NOLINTNEXTLINE(misc-include-cleaner)
#include <experimental/simd>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "quaternionTypes.hpp"
#include "vectorTypes.hpp"

namespace ammonite {
  //Maths operations
  inline namespace maths {
    /*
     - Return the address of the first element
     - Guaranteed to be the same as the quaternion, with a different type
    */
    template <typename T> requires validQuaternion<T>
    constexpr T* data(Quat<T>& a) {
      return &a[0][0];
    }

    /*
     - Return the address of the first element
     - Guaranteed to be the same as the quaternion, with a different type
     - const version of the above
    */
    template <typename T> requires validQuaternion<const T>
    constexpr const T* data(const Quat<T>& a) {
      return &a[0][0];
    }

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

    //TODO: Implement with <simd>
    //Initialise the quaternion dest with Euler angles x, y and z
    template <typename T> requires validQuaternion<T>
    void fromEuler(Quat<T>& dest, T x, T y, T z) {
      glm::vec<3, T, glm::defaultp> angles(x, y, z);
      glm::qua<T> glmQuat(angles);
      dest[0][0] = glmQuat.x;
      dest[0][1] = glmQuat.y;
      dest[0][2] = glmQuat.z;
      dest[0][3] = glmQuat.w;
    }

    //Initialise the quaternion dest with a vector of Euler angles x, y and z
    template <typename T> requires validQuaternion<T>
    void fromEuler(Quat<T>& dest, const Vec<T, 3>& angles) {
      fromEuler(dest, angles[0], angles[1], angles[2]);
    }

    //TODO: Implement with <simd>
    //Convert the quaternion src to Euler angles x, y and z, storing them in the vector dest
    template <typename T> requires validQuaternion<T>
    void toEuler(const Quat<T>& src, Vec<T, 3>& dest) {
      glm::qua<T> glmQuat(src[0][3], src[0][0], src[0][1], src[0][2]);
      glm::vec<3, T, glm::defaultp> glmAngles = glm::eulerAngles(glmQuat);
      dest[0] = glmAngles.x;
      dest[1] = glmAngles.y;
      dest[2] = glmAngles.z;
    }

    /*
     - Initialise the quaternion dest with pitch, yaw and roll angles
     - This is identical to fromEuler()
    */
    template <typename T> requires validQuaternion<T>
    void fromPitchYawRoll(Quat<T>& dest, T pitch, T yaw, T roll) {
      fromEuler(dest, pitch, yaw, roll);
    }

    /*
     - Initialise the quaternion dest with a vector of pitch, yaw and roll angles
     - This is identical to fromEuler()
    */
    template <typename T> requires validQuaternion<T>
    void fromPitchYawRoll(Quat<T>& dest, const Vec<T, 3>& angles) {
      fromEuler(dest, angles[0], angles[1], angles[2]);
    }

    /*
     - Convert the quaternion src to pitch, yaw and roll, storing them in the vector dest
     - This is identical to toEuler()
    */
    template <typename T> requires validQuaternion<T>
    void toPitchYawRoll(const Quat<T>& src, Vec<T, 3>& dest) {
      toEuler(src, dest);
    }

    //Calculate the dot product a quaternion
    template <typename T> requires validQuaternion<T>
    T dot(const Quat<T>& a, const Quat<T>& b) {
      std::experimental::fixed_size_simd<T, 4> aSimd(&a[0][0], std::experimental::element_aligned);
      std::experimental::fixed_size_simd<T, 4> bSimd(&b[0][0], std::experimental::element_aligned);

      return std::experimental::reduce(aSimd * bSimd, std::plus{});
    }

    //TODO: Implement with <simd>
    //Multiply quaternion a by quaternion b, sorting the result in dest
    template <typename T> requires validQuaternion<T>
    void multiply(const Quat<T>& a, const Quat<T>& b, Quat<T>& dest) {
      glm::qua<T> glmQuatA(a[0][3], a[0][0], a[0][1], a[0][2]);
      glm::qua<T> glmQuatB(b[0][3], b[0][0], b[0][1], b[0][2]);

      glm::qua<T> glmResult = glmQuatA * glmQuatB;

      dest[0][0] = glmResult.x;
      dest[0][1] = glmResult.y;
      dest[0][2] = glmResult.z;
      dest[0][3] = glmResult.w;
    }

    //Multiply quaternion a by quaternion b, sorting the result in the first quaternion
    template <typename T> requires validQuaternion<T>
    void multiply(Quat<T>& a, const Quat<T>& b) {
      multiply(a, b, a);
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
