#ifndef QUATERNIONTESTTEMPLATES
#define QUATERNIONTESTTEMPLATES

#include <iostream>
#include <string_view>

#include <ammonite/ammonite.hpp>

#include "../support.hpp"

namespace {
  template <typename T> requires ammonite::validQuaternion<T>
  bool testData() {
    ammonite::Quat<T> aQuat = {{0}};

    if ((void*)ammonite::data(aQuat) != (void*)&aQuat) {
      ammonite::utils::error << "Data pointer has a different address to the quaternion" << std::endl;
      ammonite::utils::normal << "  Result:   " << (void*)ammonite::data(aQuat) \
                              << "\n  Expected: " << (void*)&aQuat << std::endl;
      return false;
    }

    return true;
  }

  template <typename T> requires ammonite::validQuaternion<T>
  bool testCopy() {
    ammonite::Quat<T> aQuat = {{0}};
    ammonite::Quat<T> bQuat = {{0}};
    randomFillQuaternion(aQuat);

    ammonite::copy(aQuat, bQuat);
    for (unsigned int i = 0; i < 4; i++) {
      if (aQuat[0][i] != bQuat[0][i]) {
        ammonite::utils::error << "Quaternion copy failed" << std::endl;
        ammonite::utils::normal << "  Result:   " << ammonite::formatQuaternion(bQuat) \
                                << "\n  Expected: " << ammonite::formatQuaternion(aQuat) \
                                << std::endl;
        return false;
      }
    }

    return true;
  }

  template <typename T> requires ammonite::validQuaternion<T>
  bool testCopyCast() {
    ammonite::Quat<T> aQuat = {{0}};
    ammonite::Quat<float> bQuat = {{0}};
    ammonite::Quat<double> cQuat = {{0}};
    randomFillQuaternion(aQuat);

    ammonite::copyCast(aQuat, bQuat);
    for (unsigned int i = 0; i < 4; i++) {
      if ((float)aQuat[0][i] != bQuat[0][i]) {
        ammonite::utils::error << "Quaternion copy cast failed" << std::endl;
        ammonite::utils::normal << "  Result:   " << ammonite::formatQuaternion(bQuat) \
                                << "\n  Expected: " << ammonite::formatQuaternion(aQuat)
                                << std::endl;
        return false;
      }
    }

    ammonite::copyCast(aQuat, cQuat);
    for (unsigned int i = 0; i < 4; i++) {
      if ((double)aQuat[0][i] != cQuat[0][i]) {
        ammonite::utils::error << "Quaternion copy cast failed" << std::endl;
        ammonite::utils::normal << "  Result:   " << ammonite::formatQuaternion(cQuat) \
                                << "\n  Expected: " << ammonite::formatQuaternion(aQuat)
                                << std::endl;
        return false;
      }
    }

    return true;
  }
}

namespace tests {
  template <typename T> requires ammonite::validQuaternion<T>
  bool testQuaternion(std::string_view typeName) {
    ammonite::utils::normal << "Testing " << typeName << " quaternions" << std::endl;

    //Test ammonite::data()
    if (!testData<T>()) {
      return false;
    }

    for (int i = 0; i < 10000; i++) {
      //Test ammonite::copy()
      if (!testCopy<T>()) {
        return false;
      }

      //Test ammonite::copyCast()
      if (!testCopyCast<T>()) {
        return false;
      }
    }

    return true;
  }
}

namespace tests {
  bool testFloatQuat();
  bool testDoubleQuat();
}

#endif
