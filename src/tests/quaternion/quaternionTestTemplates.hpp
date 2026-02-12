#ifndef QUATERNIONTESTTEMPLATES
#define QUATERNIONTESTTEMPLATES

#include <cmath>
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
  bool testInit() {
    //NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
    struct TestData {
      T xAngle;
      T yAngle;
      T zAngle;
      ammonite::Quat<T> out;
    };
    //NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)

    const TestData tests[9] = {
      //No rotation
      {(T)0.0, (T)0.0, (T)0.0, {{(T)0.0, (T)0.0, (T)0.0, (T)1.0}}},

      //Complete rotation in each axis
      {(T)2.0 * ammonite::pi<T>, (T)0.0, (T)0.0, {{(T)0.0, (T)0.0, (T)0.0, (T)-1.0}}},
      {(T)0.0, (T)2.0 * ammonite::pi<T>, (T)0.0, {{(T)0.0, (T)0.0, (T)0.0, (T)-1.0}}},
      {(T)0.0, (T)0.0, (T)2.0 * ammonite::pi<T>, {{(T)0.0, (T)0.0, (T)0.0, (T)-1.0}}},
      {(T)2.0 * ammonite::pi<T>, (T)2.0 * ammonite::pi<T>, (T)2.0 * ammonite::pi<T>, {{(T)0.0, (T)0.0, (T)0.0, (T)-1.0}}},

      //Half rotation in each axis
      {ammonite::pi<T>, (T)0.0, (T)0.0, {{(T)1.0, (T)0.0, (T)0.0, (T)0.0}}},
      {(T)0.0, ammonite::pi<T>, (T)0.0, {{(T)0.0, (T)1.0, (T)0.0, (T)0.0}}},
      {(T)0.0, (T)0.0, ammonite::pi<T>, {{(T)0.0, (T)0.0, (T)1.0, (T)0.0}}},
      {ammonite::pi<T>, ammonite::pi<T>, ammonite::pi<T>, {{(T)0.0, (T)0.0, (T)0.0, (T)1.0}}}
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
      const ammonite::Vec<T, 3> angleVec = {
        tests[testIndex].xAngle, tests[testIndex].yAngle, tests[testIndex].zAngle
      };
      ammonite::fromEuler(outQuatAlt, angleVec);
      for (int i = 0; i < 3; i++) {
        if (outQuatAlt[0][i] != outQuat[0][i]) {
          ammonite::utils::error << "Quaternion Euler angle vector initialisation failed" << std::endl;
          ammonite::utils::normal << "  Result:   " << ammonite::formatQuaternion(outQuatAlt) \
                                  << "\n  Expected: " << ammonite::formatQuaternion(outQuat) \
                                  << std::endl;
          return false;
        }
      }
    }

    return true;
  }

  template <typename T> requires ammonite::validQuaternion<T>
  bool testToEuler() {
    if constexpr (sizeof(T) >= 8) {
      ammonite::Quat<T> aQuat = {{0}};
      ammonite::Vec<T, 3> aVec = {0};
      ammonite::Vec<T, 3> angleVec = {0};

      //Initialise a random angle vector and corresponding quaternion
      ammonite::fromEuler(aQuat, randomFillVector(angleVec, (T)2.0 * ammonite::pi<T>));

      //Get angles out of the quaternion
      ammonite::toEuler(aQuat, aVec);

      //Check the angles match
      T sumA = (T)1.0;
      T sumB = (T)1.0;
      for (int i = 0; i < 3; i++) {
        sumA *= std::sin(aVec[i]);
        sumB *= std::sin(angleVec[i]);
      }

      if (!roughly(sumA, sumB)) {
        ammonite::utils::error << "Quaternion Euler angle recovery failed" << std::endl;
        ammonite::utils::normal << "  Result:   " << ammonite::formatVector(aVec) \
                                << "\n  Expected: " << ammonite::formatVector(angleVec) \
                                << std::endl;
        return false;
      }
    }

    return true;
  }

  template <typename T> requires ammonite::validQuaternion<T>
  bool testDot() {
    ammonite::Quat<T> aQuat = {{0}};
    ammonite::Quat<T> bQuat = {{0}};
    ammonite::Vec<T, 3> angleVecA = {0};
    ammonite::Vec<T, 3> angleVecB = {0};

    //Initialise two pairs of random angle vectors and corresponding quaternions
    ammonite::fromEuler(aQuat, randomFillVector(angleVecA, (T)2.0 * ammonite::pi<T>));
    ammonite::fromEuler(bQuat, randomFillVector(angleVecB, (T)2.0 * ammonite::pi<T>));

    T sum = (T)0;
    for (int i = 0; i < 4; i++) {
      sum += aQuat[0][i] * bQuat[0][i];
    }

    //Test dot product
    if (!roughly(ammonite::dot(aQuat, bQuat), sum)) {
      ammonite::utils::error << "Quaternion dot product failed" << std::endl;
      ammonite::utils::normal << "  Input:    " << ammonite::formatQuaternion(aQuat) \
                              << "\n  Input:    " << ammonite::formatQuaternion(bQuat) \
                              << "\n  Result:   " << ammonite::dot(aQuat, bQuat) \
                              << "\n  Expected: " << sum << std::endl;
      return false;
    }

    return true;
  }

  template <typename T> requires ammonite::validQuaternion<T>
  bool testConjugate() {
    ammonite::Quat<T> aQuat = {{0}};
    ammonite::Quat<T> bQuat = {{0}};
    ammonite::Quat<T> cQuat = {{0}};
    ammonite::Vec<T, 3> angleVec = {0};
    ammonite::fromEuler(aQuat, randomFillVector(angleVec, (T)2.0 * ammonite::pi<T>));

    //Test conjugate calculation
    ammonite::conjugate(aQuat, bQuat);
    bool valid = true;
    for (int i = 0; i < 3; i++) {
      if (-aQuat[0][i] != bQuat[0][i]) {
        valid = false;
        break;
      }
    }

    if (!valid || aQuat[0][3] != bQuat[0][3]) {
      ammonite::utils::error << "Quaternion conjugate calculation failed" << std::endl;
      ammonite::utils::normal << "  Input:  " << ammonite::formatQuaternion(aQuat) \
                              << "\n  Result: " << ammonite::formatQuaternion(bQuat) \
                              << std::endl;
      return false;
    }

    //Test in-place conjugate calculation
    ammonite::copy(aQuat, cQuat);
    ammonite::conjugate(aQuat);
    for (int i = 0; i < 4; i++) {
      if (aQuat[0][i] != bQuat[0][i]) {
        ammonite::utils::error << "In-place quaternion conjugate calculation failed" << std::endl;
        ammonite::utils::normal << "  Input:    " << ammonite::formatQuaternion(cQuat) \
                                << "\n  Result:   " << ammonite::formatQuaternion(aQuat) \
                                << "\n  Expected: " << ammonite::formatQuaternion(bQuat) \
                                << std::endl;
        return false;
      }
    }

    return true;
  }

  template <typename T> requires ammonite::validQuaternion<T>
  bool testLength() {
    ammonite::Quat<T> aQuat = {{0}};
    ammonite::Vec<T, 3> angleVec = {0};
    ammonite::fromEuler(aQuat, randomFillVector(angleVec, (T)2.0 * ammonite::pi<T>));

    T sum = (T)0;
    for (int i = 0; i < 4; i++) {
      sum += aQuat[0][i] * aQuat[0][i];
    }
    T length = (T)std::sqrt(sum);

    //Test vector length
    if (!roughly(ammonite::length(aQuat), length)) {
      ammonite::utils::error << "Quaternion length calculation failed" << std::endl;
      ammonite::utils::normal << "  Input:    " << ammonite::formatQuaternion(aQuat) \
                              << "\n  Result:   " << ammonite::length(aQuat) \
                              << "\n  Expected: " << length << std::endl;

      return false;
    }

    return true;
  }

  template <typename T> requires ammonite::validQuaternion<T>
  bool testNormalise() {
    ammonite::Quat<T> aQuat = {{0}};
    ammonite::Quat<T> bQuat = {{0}};
    ammonite::Vec<T, 3> angleVec = {0};
    ammonite::fromEuler(aQuat, randomFillVector(angleVec, (T)2.0 * ammonite::pi<T>));

    //Skip (effectively) zero length quaternions
    T length = ammonite::length(aQuat);
    if (length == 0) {
      return true;
    }

    //Test regular normalisation
    ammonite::normalise(aQuat, bQuat);
    for (int i = 0; i < 4; i++) {
      if (!roughly((T)(aQuat[0][i] / length), bQuat[0][i])) {
        ammonite::utils::error << "Quaternion normalisation failed" << std::endl;
        ammonite::utils::normal << "  Input:    " << ammonite::formatQuaternion(aQuat) \
                                << "\n  Result:   " << ammonite::formatQuaternion(bQuat) \
                                << "\n  Expected: " << (T)(aQuat[0][i] / length) \
                                << " at index " << i << std::endl;
        return false;
      }
    }

    //Test in-place normalisation
    ammonite::copy(aQuat, bQuat);
    ammonite::normalise(bQuat);
    for (int i = 0; i < 4; i++) {
      if (!roughly((T)(aQuat[0][i] / length), bQuat[0][i])) {
        ammonite::utils::error << "In-place quaternion normalisation failed" << std::endl;
        ammonite::utils::normal << "  Input:    " << ammonite::formatQuaternion(aQuat) \
                                << "\n  Result:   " << ammonite::formatQuaternion(bQuat) \
                                << "\n  Expected: " << (T)(aQuat[0][i] / length) \
                                << " at index " << i << std::endl;
        return false;
      }
    }

    return true;
  }

  template <typename T> requires ammonite::validQuaternion<T>
  bool testInverse() {
    ammonite::Quat<T> aQuat = {{0}};
    ammonite::Quat<T> bQuat = {{0}};
    ammonite::Quat<T> cQuat = {{0}};
    ammonite::Quat<T> dQuat = {{0}};
    ammonite::Vec<T, 3> angleVecA = {0};
    ammonite::Vec<T, 3> angleVecB = {0};

    ammonite::fromEuler(aQuat, randomFillVector(angleVecA, (T)2.0 * ammonite::pi<T>));
    ammonite::fromEuler(bQuat, randomFillVector(angleVecB, (T)2.0 * ammonite::pi<T>));

    ammonite::inverse(aQuat, cQuat);

    //Check that multiplying aQuat by its inverse gives no rotation
    ammonite::multiply(aQuat, cQuat, dQuat);
    const ammonite::Quat<T> expectedQuat = {{(T)0.0, (T)0.0, (T)0.0, (T)1.0}};
    for (int i = 0; i < 4; i++) {
      if (!roughly(dQuat[0][i], expectedQuat[0][i])) {
        ammonite::utils::error << "Quaternion inverse failed" << std::endl;
        ammonite::utils::normal << "  Input:  " << ammonite::formatQuaternion(aQuat) \
                                << "\n  Result: " << ammonite::formatQuaternion(cQuat) \
                                << std::endl;
        return false;
      }
    }

    //Test in-place quaternion inverse
    ammonite::copy(aQuat, bQuat);
    ammonite::inverse(aQuat);
    for (int i = 0; i < 4; i++) {
      if (!roughly(aQuat[0][i], cQuat[0][i])) {
        ammonite::utils::error << "In-place quaternion inverse failed" << std::endl;
        ammonite::utils::normal << "  Input:    " << ammonite::formatQuaternion(bQuat) \
                                << "\n  Result:   " << ammonite::formatQuaternion(aQuat) \
                                << "\n  Expected: " << ammonite::formatQuaternion(cQuat) \
                                << std::endl;
        return false;
      }
    }

    return true;
  }

  template <typename T> requires ammonite::validQuaternion<T>
  bool testMultiplyQuat() {
    ammonite::Quat<T> aQuat = {{0}};
    ammonite::Quat<T> bQuat = {{0}};
    ammonite::Quat<T> cQuat = {{0}};
    ammonite::Quat<T> dQuat = {{0}};
    ammonite::Quat<T> eQuat = {{0}};
    ammonite::Quat<T> fQuat = {{0}};
    ammonite::Vec<T, 3> angleVec = {0};
    randomFillVector(angleVec, (T)2.0 * ammonite::pi<T>);

    //Initialise quaternions using the angles
    ammonite::fromEuler(aQuat, angleVec[0], (T)0.0, (T)0.0);
    ammonite::fromEuler(bQuat, (T)0.0, angleVec[1], (T)0.0);
    ammonite::fromEuler(cQuat, (T)0.0, (T)0.0, angleVec[2]);

    //Multiply each axis quaternion together
    ammonite::multiply(cQuat, ammonite::multiply(bQuat, aQuat, dQuat), eQuat);

    //Test multiplied quaternion matches initialised quaternion
    ammonite::fromEuler(fQuat, angleVec);
    if (!roughly(std::abs(ammonite::dot(fQuat, eQuat)), (T)1.0)) {
      ammonite::utils::error << "Quaternion-quaternion multiplication failed" << std::endl;
      ammonite::utils::normal << "  Result:   " << ammonite::formatQuaternion(eQuat) \
                              << "\n  Expected: " << ammonite::formatQuaternion(fQuat) \
                              << std::endl;
      return false;
    }

    //Test the in-place variant matches the regular variant
    ammonite::multiply(cQuat, ammonite::multiply(bQuat, aQuat));
    for (int i = 0; i < 4; i++) {
      if (cQuat[0][i] != eQuat[0][i]) {
        ammonite::utils::error << "In-place quaternion-quaternion multiplication failed" << std::endl;
        ammonite::utils::normal << "  Result:   " << ammonite::formatQuaternion(cQuat) \
                                << "\n  Expected: " << ammonite::formatQuaternion(eQuat) \
                                << std::endl;
        return false;
      }
    }

    return true;
  }

  template <typename T> requires ammonite::validQuaternion<T>
  bool testMultiplyVec() {
    //NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
    struct TestData {
      const ammonite::Vec<T, 3> angleVec;
      ammonite::Vec<T, 3> in;
      ammonite::Vec<T, 3> out;
    };
    //NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)

    TestData tests[8] = {
      { //Convert between axes
        {-ammonite::pi<T> / 2, (T)0.0, (T)0.0},
        {(T)0.0, (T)0.0, (T)1.0},
        {(T)0.0, (T)1.0, (T)0.0}
      }, {
        {(T)0.0, -ammonite::pi<T> / 2, (T)0.0},
        {(T)1.0, (T)0.0, (T)0.0},
        {(T)0.0, (T)0.0, (T)1.0}
      }, {
        {(T)0.0, (T)0.0, -ammonite::pi<T> / 2},
        {(T)0.0, (T)1.0, (T)0.0},
        {(T)1.0, (T)0.0, (T)0.0}
      },

      { //Complete turns
        {ammonite::pi<T> * 2, ammonite::pi<T> * 2, ammonite::pi<T> * 2},
        {(T)1.0, (T)0.0, (T)0.0},
        {(T)1.0, (T)0.0, (T)0.0}
      }, {
        {ammonite::pi<T> * 2, ammonite::pi<T> * 2, ammonite::pi<T> * 2},
        {(T)0.0, (T)1.0, (T)0.0},
        {(T)0.0, (T)1.0, (T)0.0}
      }, {
        {ammonite::pi<T> * 2, ammonite::pi<T> * 2, ammonite::pi<T> * 2},
        {(T)0.0, (T)0.0, (T)1.0},
        {(T)0.0, (T)0.0, (T)1.0}
      },

      { //Recover x and z from xz
        {(T)0.0, ammonite::pi<T> / 4, (T)0.0},
        {(T)1.0, (T)0.0, (T)1.0},
        {(T)1.0, (T)0.0, (T)0.0}
      }, {
        {(T)0.0, -ammonite::pi<T> / 4, (T)0.0},
        {(T)1.0, (T)0.0, (T)1.0},
        {(T)0.0, (T)0.0, (T)1.0}
      }
    };

    const int totalTests = sizeof(tests) / sizeof(TestData);
    for (int testIndex = 0; testIndex < totalTests; testIndex++) {
      //Prepare the quaternion
      ammonite::Quat<T> aQuat = {{0}};
      ammonite::fromEuler(aQuat, tests[testIndex].angleVec);

      //Normalise test data
      ammonite::normalise(tests[testIndex].in);
      ammonite::normalise(tests[testIndex].out);

      //Rotate the point
      ammonite::Vec<T, 3> outVec = {0};
      ammonite::multiply(aQuat, tests[testIndex].in, outVec);

      //Check the rotated point matches the expected
      for (int i = 0; i < 3; i++) {
        if (!roughly(outVec[i], tests[testIndex].out[i])) {
          ammonite::utils::error << "Quaternion-vector multiplication failed" << std::endl;
          ammonite::utils::normal << "  Quaternion: " << ammonite::formatQuaternion(aQuat) \
                                  << "\n  Result:     " << ammonite::formatVector(outVec) \
                                  << "\n  Expected:   " << ammonite::formatVector(tests[testIndex].out) \
                                  << std::endl;
          return false;
        }
      }

      //Repeat with a widened vector
      ammonite::Vec<T, 4> outVecWide = {0};
      ammonite::Vec<T, 4> inVecWide = {0};
      ammonite::set(inVecWide, tests[testIndex].in, (T)1.0);
      ammonite::multiply(aQuat, inVecWide, outVecWide);

      //Check the wider rotated point matches the expected
      ammonite::Vec<T, 4> expectedOutVecWide = {0};
      ammonite::set(expectedOutVecWide, tests[testIndex].out, (T)1.0);
      for (int i = 0; i < 4; i++) {
        if (!roughly(outVecWide[i], expectedOutVecWide[i])) {
          ammonite::utils::error << "Quaternion-vector multiplication failed" << std::endl;
          ammonite::utils::normal << "  Quaternion: " << ammonite::formatQuaternion(aQuat) \
                                  << "\n  Result:     " << ammonite::formatVector(outVecWide) \
                                  << "\n  Expected:   " << ammonite::formatVector(expectedOutVecWide) \
                                  << std::endl;
          return false;
        }
      }

      //Repeat regular length test in-place
      ammonite::Vec<T, 3> inVec = {0};
      ammonite::copy(tests[testIndex].in, inVec);
      ammonite::multiply(aQuat, inVec);
      if (!ammonite::equal(inVec, outVec)) {
        ammonite::utils::error << "Quaternion-vector multiplication failed" << std::endl;
        ammonite::utils::normal << "  Quaternion: " << ammonite::formatQuaternion(aQuat) \
                                << "\n  Result:     " << ammonite::formatVector(inVec) \
                                << "\n  Expected:   " << ammonite::formatVector(outVec) \
                                << std::endl;
        return false;
      }

      //Repeat wide length test in-place
      ammonite::Vec<T, 4> inVecWideInplace = {0};
      ammonite::copy(inVecWide, inVecWideInplace);
      ammonite::multiply(aQuat, inVecWideInplace);
      if (!ammonite::equal(inVecWideInplace, outVecWide)) {
        ammonite::utils::error << "Quaternion-vector multiplication failed" << std::endl;
        ammonite::utils::normal << "  Quaternion: " << ammonite::formatQuaternion(aQuat) \
                                << "\n  Result:     " << ammonite::formatVector(inVecWideInplace) \
                                << "\n  Expected:   " << ammonite::formatVector(outVecWide) \
                                << std::endl;
        return false;
      }
    }

    return true;
  }

  template <typename T> requires ammonite::validQuaternion<T>
  bool testToMatrix() {
    ammonite::Quat<T> aQuat = {{0}};
    ammonite::Vec<T, 4> aVec = {0};
    ammonite::Vec<T, 4> bVec = {0};
    ammonite::Vec<T, 4> cVec = {0};
    ammonite::Vec<T, 3> angleVec = {0};
    randomFillVector(aVec);
    ammonite::fromEuler(aQuat, randomFillVector(angleVec, (T)2.0 * ammonite::pi<T>));

    //Apply the quaternion to a point
    ammonite::multiply(aQuat, aVec, bVec);

    //Calculate rotation matrix equivalent, apply to another point
    ammonite::Mat<T, 4> aMat = {{0}};
    ammonite::multiply(ammonite::toMatrix(aQuat, aMat), aVec, cVec);

    //Check 4x4 matrix conversion results
    for (int i = 0; i < 4; i++) {
      if (!roughly(bVec[i], cVec[i], (T)0.01)) {
        ammonite::utils::error << "Quaternion to 4x4 matrix conversion failed" << std::endl;
        ammonite::utils::normal << "  Quaternion: " << ammonite::formatQuaternion(aQuat) \
                                << "\n  Matrix result:\n" << ammonite::formatMatrix(aMat) \
                                << "\n  Input vector:\n" << ammonite::formatVector(aVec) \
                                << "\n  Quaternion output vector:\n" << ammonite::formatVector(bVec) \
                                << "\n  Matrix output vector:\n" << ammonite::formatVector(cVec) \
                                << std::endl;
        return false;
      }
    }

    //Repeat with a 3x3 matrix
    ammonite::Vec<T, 3> aVecShort = {0};
    ammonite::Vec<T, 3> bVecShort = {0};
    ammonite::Vec<T, 3> cVecShort = {0};
    randomFillVector(aVecShort);

    //Apply the quaternion to a point
    ammonite::multiply(aQuat, aVecShort, bVecShort);

    //Calculate rotation matrix equivalent, apply to another point
    ammonite::Mat<T, 3> bMat = {{0}};
    ammonite::toMatrix(aQuat, bMat);
    ammonite::multiply(bMat, aVecShort, cVecShort);

    //Check 3x3 matrix conversion results
    for (int i = 0; i < 3; i++) {
      if (!roughly(bVecShort[i], cVecShort[i], (T)0.01)) {
        ammonite::utils::error << "Quaternion to 3x3 matrix conversion failed" << std::endl;
        ammonite::utils::normal << "  Quaternion: " << ammonite::formatQuaternion(aQuat) \
                                << "\n  Matrix result:\n" << ammonite::formatMatrix(bMat) \
                                << "\n  Input vector:\n" << ammonite::formatVector(aVecShort) \
                                << "\n  Quaternion output vector:\n" << ammonite::formatVector(bVecShort) \
                                << "\n  Matrix output vector:\n" << ammonite::formatVector(cVecShort) \
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

      //Test ammonite::toEuler()
      if (!testToEuler<T>()) {
        return false;
      }

      //Test ammonite::dot()
      if (!testDot<T>()) {
        return false;
      }

      //Test ammonite::conjugate()
      if (!testConjugate<T>()) {
        return false;
      }

      //Test ammonite::length()
      if (!testLength<T>()) {
        return false;
      }

      //Test ammonite::normalise()
      if (!testNormalise<T>()) {
        return false;
      }

      //Test ammonite::inverse()
      if (!testInverse<T>()) {
        return false;
      }

      //Test ammonite::multiply() for quaternion-quaternion
      if (!testMultiplyQuat<T>()) {
        return false;
      }

      //Test ammonite::toMatrix()
      if (!testToMatrix<T>()) {
        return false;
      }
    }

    //Test ammonite::multiply() for quaternion-vector
    if (!testMultiplyVec<T>()) {
      return false;
    }

    //Test ammonite::fromEuler()
    return testInit<T>();
  }
}

namespace tests {
  bool testFloatQuat();
  bool testDoubleQuat();
}

#endif
