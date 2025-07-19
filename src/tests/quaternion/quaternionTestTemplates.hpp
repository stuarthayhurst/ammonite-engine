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
                                << "\n  Expected: " << ammonite::formatQuaternion(aQuat) \
                                << std::endl;
        return false;
      }
    }

    ammonite::copyCast(aQuat, cQuat);
    for (unsigned int i = 0; i < 4; i++) {
      if ((double)aQuat[0][i] != cQuat[0][i]) {
        ammonite::utils::error << "Quaternion copy cast failed" << std::endl;
        ammonite::utils::normal << "  Result:   " << ammonite::formatQuaternion(cQuat) \
                                << "\n  Expected: " << ammonite::formatQuaternion(aQuat) \
                                << std::endl;
        return false;
      }
    }

    return true;
  }

  template <typename T> requires ammonite::validQuaternion<T>
  bool testEuler() {
    //NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
    struct TestData {
      T xAngle;
      T yAngle;
      T zAngle;
      ammonite::Quat<T> out;
    };
    //NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)

    TestData tests[9] = {
      //No rotation
      {(T)0.0, (T)0.0, (T)0.0, {{(T)0.0, (T)0.0, (T)0.0, (T)1.0}}},

      //Complete rotation in each axis
      {(T)2.0 * ammonite::pi<T>(), (T)0.0, (T)0.0, {{(T)0.0, (T)0.0, (T)0.0, (T)-1.0}}},
      {(T)0.0, (T)2.0 * ammonite::pi<T>(), (T)0.0, {{(T)0.0, (T)0.0, (T)0.0, (T)-1.0}}},
      {(T)0.0, (T)0.0, (T)2.0 * ammonite::pi<T>(), {{(T)0.0, (T)0.0, (T)0.0, (T)-1.0}}},
      {(T)2.0 * ammonite::pi<T>(), (T)2.0 * ammonite::pi<T>(), (T)2.0 * ammonite::pi<T>(), {{(T)0.0, (T)0.0, (T)0.0, (T)-1.0}}},

      //Half rotation in each axis
      {ammonite::pi<T>(), (T)0.0, (T)0.0, {{(T)1.0, (T)0.0, (T)0.0, (T)0.0}}},
      {(T)0.0, ammonite::pi<T>(), (T)0.0, {{(T)0.0, (T)1.0, (T)0.0, (T)0.0}}},
      {(T)0.0, (T)0.0, ammonite::pi<T>(), {{(T)0.0, (T)0.0, (T)1.0, (T)0.0}}},
      {ammonite::pi<T>(), ammonite::pi<T>(), ammonite::pi<T>(), {{(T)0.0, (T)0.0, (T)0.0, (T)1.0}}}
    };

    const int totalTests = sizeof(tests) / sizeof(TestData);
    for (int testIndex = 0; testIndex < totalTests; testIndex++) {
      //Prepare quaternion storage
      ammonite::Quat<T> outQuat = {{0}};

      //Initialise the quaternion
      ammonite::fromEuler(outQuat, tests[testIndex].xAngle,
                          tests[testIndex].yAngle, tests[testIndex].zAngle);

      //Compare the result to the expected
      for (int i = 0; i < 4; i++) {
        if (!roughly(outQuat[0][i], tests[testIndex].out[0][i])) {
          ammonite::utils::error << "Quaternion Euler angle initialisation failed" << std::endl;
          ammonite::utils::normal << "  Result:   " << ammonite::formatQuaternion(outQuat) \
                                  << "\n  Expected: " << ammonite::formatQuaternion(tests[testIndex].out) \
                                  << std::endl;
          return false;
        }
      }

      //Check the same results are given from a vector of angles
      ammonite::Quat<T> outQuatAlt = {{0}};
      ammonite::Vec<T, 3> angleVec = {
        tests[testIndex].xAngle, tests[testIndex].yAngle, tests[testIndex].zAngle
      };
      ammonite::fromEuler(outQuatAlt, angleVec);
      for (int i = 0; i < 3; i++) {
        if (outQuatAlt[0][i] != outQuat[0][i]) {
          ammonite::utils::error << "Quaternion Euler angle vector initialisation failed" << std::endl;
          ammonite::utils::normal << "  Result:   " << ammonite::formatQuaternion(outQuatAlt) \
                                  << "\n  Expected: " << ammonite::formatQuaternion(outQuatAlt) \
                                  << std::endl;
          return false;
        }
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

    //Test ammonite::fromEuler()
    return testEuler<T>();
  }
}

namespace tests {
  bool testFloatQuat();
  bool testDoubleQuat();
}

#endif
