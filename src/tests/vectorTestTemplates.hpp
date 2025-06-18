#ifndef VECTORTESTTEMPLATES
#define VECTORTESTTEMPLATES

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <string_view>
#include <type_traits>

#include <ammonite/ammonite.hpp>

//Test helpers
namespace {
  template <typename T, std::size_t size> requires ammonite::validVector<T, size>
  void randomFillVector(ammonite::Vec<T, size>& vec) {
    for (std::size_t i = 0; i < size; i++) {
      if constexpr (std::is_unsigned_v<T> && !std::is_same_v<T, unsigned char> &&
                    !std::is_same_v<T, unsigned short>) {
        vec[i] = (T)ammonite::utils::randomUInt(std::numeric_limits<T>::max());
      } else if constexpr (std::is_integral_v<T>) {
        vec[i] = (T)ammonite::utils::randomInt((T)std::sqrt(std::numeric_limits<T>::max() / size));
      } else {
        vec[i] = (T)ammonite::utils::randomDouble(std::numeric_limits<T>::max());
      }
    }
  }

  template <typename T, std::size_t size, typename S> requires ammonite::validVector<T, size>
  void reportFailure(std::string_view message, const ammonite::Vec<T, size>& a, S b) {
    ammonite::utils::error << message << std::endl;
    ammonite::utils::error << a << std::endl;
    ammonite::utils::error << b << std::endl;
    ammonite::utils::error << std::endl;
  }

  template <typename T, std::size_t sizeA, typename S, std::size_t sizeB> requires ammonite::validVector<T, sizeA> && ammonite::validVector<S, sizeB>
  void reportFailure(std::string_view message, const ammonite::Vec<T, sizeA>& a, const ammonite::Vec<S, sizeB>& b) {
    ammonite::utils::error << message << std::endl;
    ammonite::utils::error << a << std::endl;
    ammonite::utils::error << b << std::endl;
    ammonite::utils::error << std::endl;
  }

  template <typename T, std::size_t sizeA, typename S, std::size_t sizeB, typename U, std::size_t sizeC> requires ammonite::validVector<T, sizeA> && ammonite::validVector<S, sizeB> && ammonite::validVector<U, sizeC>
  void reportFailure(std::string_view message, const ammonite::Vec<T, sizeA>& a,
                     const ammonite::Vec<S, sizeB>& b, const ammonite::Vec<U, sizeC>& c) {
    ammonite::utils::error << message << std::endl;
    ammonite::utils::error << a << std::endl;
    ammonite::utils::error << b << std::endl;
    ammonite::utils::error << c << std::endl;
    ammonite::utils::error << std::endl;
  }
}

//Tests
namespace {
  template <typename T, std::size_t size> requires ammonite::validVector<T, size>
  bool testNamedVec() {
    ammonite::Vec<T, size> aVec = {0};
    ammonite::NamedVec<T, size> namedAVec(aVec);

    if (&namedAVec.x != aVec.data()) {
      ammonite::utils::error << "Named vector has a different address to its underlying vector" << std::endl;
      ammonite::utils::error << "Named vector address: " << (void*)&namedAVec.x \
                              << ", vector address: " << (void*)aVec.data() << std::endl;
      ammonite::utils::error << std::endl;
      return false;
    }

    return true;
  }

  template <typename T, std::size_t size> requires ammonite::validVector<T, size>
  bool testEqual() {
    ammonite::Vec<T, size> aVec = {0};
    ammonite::Vec<T, size> bVec = {0};
    randomFillVector(aVec);

    //Set bVec to aVec
    for (std::size_t e = 0; e < size; e++) {
      bVec[e] = aVec[e];
    }

    //Check ammonite::equal() on equal vectors
    if (!ammonite::equal(aVec, bVec)) {
      reportFailure("Equal vector comparison failed", aVec, bVec);
      return false;
    }

    //Safely guarantee a modification to bVec
    uintmax_t tmp = 0;
    std::memcpy(&tmp, bVec.data(), sizeof(bVec[0]));
    tmp ^= 1;
    std::memcpy(bVec.data(), &tmp, sizeof(bVec[0]));

    //Check ammonite::equal() on unequal vectors
    if (ammonite::equal(aVec, bVec)) {
      reportFailure("Enequal vector comparison failed", aVec, bVec);
      return false;
    }

    return true;
  }

  template <typename T, std::size_t size> requires ammonite::validVector<T, size>
  bool testCopy() {
    ammonite::Vec<T, size> aVec = {0};
    ammonite::Vec<T, size> bVec = {0};
    randomFillVector(aVec);
    ammonite::copy(aVec, bVec);

    if (!ammonite::equal(aVec, bVec)) {
      reportFailure("Vector copy failed", aVec, bVec);
      return false;
    }

    //Check vectors are fully preserved when copying to a max size vector
    ammonite::Vec<T, 4> cVec = {0};
    ammonite::copy(aVec, cVec);
    ammonite::copy(cVec, aVec);
    if (!ammonite::equal(aVec, bVec)) {
      reportFailure("Vector grow copy failed", aVec, bVec);
      return false;
    }

    //Check relevant parts are preserved when copying to a min size vector
    ammonite::Vec<T, 2> dVec = {0};
    ammonite::copy(aVec, dVec);
    ammonite::copy(dVec, aVec);
    if (aVec[0] != bVec[0] || aVec[1] != bVec[1]) {
      reportFailure("Vector shrink copy failed", aVec, bVec);
      return false;
    }

    return true;
  }

  template <typename T, std::size_t size> requires ammonite::validVector<T, size>
  bool testCopyCast() {
    ammonite::Vec<T, size> aVec = {0};
    ammonite::Vec<double, size> bVec = {0};
    randomFillVector(aVec);

    ammonite::copyCast(aVec, bVec);
    for (std::size_t i = 0; i < size; i++) {
      if ((double)aVec[i] != bVec[i]) {
        reportFailure("Vector copy cast failed", aVec, bVec);
        return false;
      }
    }

    //Check vectors are fully preserved when copying to a max size vector
    ammonite::Vec<double, 4> cVec = {0};
    ammonite::copyCast(aVec, cVec);

    for (std::size_t i = 0; i < size; i++) {
      if ((double)aVec[i] != cVec[i]) {
        reportFailure("Vector grow copy cast failed", aVec, cVec);
        return false;
      }
    }

    //Check relevant parts are preserved when copying to a min size vector
    ammonite::Vec<T, 2> dVec = {0};
    ammonite::copy(aVec, dVec);
    if ((double)aVec[0] != dVec[0] || (double)aVec[1] != dVec[1]) {
      reportFailure("Vector shrink copy cast failed", aVec, dVec);
      return false;
    }

    return true;
  }

  template <typename T, std::size_t size> requires ammonite::validVector<T, size>
  bool testAdd() {
    ammonite::Vec<T, size> aVec = {0};
    ammonite::Vec<T, size> bVec = {0};
    ammonite::Vec<T, size> cVec = {0};
    randomFillVector(aVec);
    randomFillVector(bVec);

    //Test regular addition
    ammonite::add(aVec, bVec, cVec);
    for (std::size_t i = 0; i < size; i++) {
      if ((T)(aVec[i] + bVec[i]) != cVec[i]) {
        reportFailure("Vector addition failed", aVec, bVec, cVec);
        return false;
      }
    }

    //Test in-place addition
    ammonite::copy(aVec, cVec);
    ammonite::add(cVec, bVec);
    for (std::size_t i = 0; i < size; i++) {
      if ((T)(aVec[i] + bVec[i]) != cVec[i]) {
        reportFailure("In-place vector addition failed", aVec, bVec, cVec);
        return false;
      }
    }

    //Test constant addition
    ammonite::add(aVec, bVec[0], cVec);
    for (std::size_t i = 0; i < size; i++) {
      if ((T)(aVec[i] + bVec[0]) != cVec[i]) {
        reportFailure("Constant vector addition failed", aVec, bVec, cVec);
        return false;
      }
    }

    //Test in-place constant addition
    ammonite::copy(aVec, cVec);
    ammonite::add(cVec, bVec[0]);
    for (std::size_t i = 0; i < size; i++) {
      if ((T)(aVec[i] + bVec[0]) != cVec[i]) {
        reportFailure("In-place constant vector addition failed", aVec, bVec, cVec);
        return false;
      }
    }

    return true;
  }

  template <typename T, std::size_t size> requires ammonite::validVector<T, size>
  bool testSub() {
    ammonite::Vec<T, size> aVec = {0};
    ammonite::Vec<T, size> bVec = {0};
    ammonite::Vec<T, size> cVec = {0};
    randomFillVector(aVec);
    randomFillVector(bVec);

    //Test regular subtraction
    ammonite::sub(aVec, bVec, cVec);
    for (std::size_t i = 0; i < size; i++) {
      if ((T)(aVec[i] - bVec[i]) != cVec[i]) {
        reportFailure("Vector subtraction failed", aVec, bVec, cVec);
        return false;
      }
    }

    //Test in-place subtraction
    ammonite::copy(aVec, cVec);
    ammonite::sub(cVec, bVec);
    for (std::size_t i = 0; i < size; i++) {
      if ((T)(aVec[i] - bVec[i]) != cVec[i]) {
        reportFailure("In-place vector subtraction failed", aVec, bVec, cVec);
        return false;
      }
    }

    //Test constant subtraction
    ammonite::sub(aVec, bVec[0], cVec);
    for (std::size_t i = 0; i < size; i++) {
      if ((T)(aVec[i] - bVec[0]) != cVec[i]) {
        reportFailure("Constant vector subtraction failed", aVec, bVec, cVec);
        return false;
      }
    }

    //Test in-place constant subtraction
    ammonite::copy(aVec, cVec);
    ammonite::sub(cVec, bVec[0]);
    for (std::size_t i = 0; i < size; i++) {
      if ((T)(aVec[i] - bVec[0]) != cVec[i]) {
        reportFailure("In-place constant vector subtraction failed", aVec, bVec, cVec);
        return false;
      }
    }

    return true;
  }

  template <typename T, std::size_t size> requires ammonite::validVector<T, size>
  bool testScale() {
    ammonite::Vec<T, size> aVec = {0};
    ammonite::Vec<T, size> bVec = {0};
    ammonite::Vec<T, size> cVec = {0};
    randomFillVector(aVec);
    randomFillVector(bVec);

    //Test regular scaling
    ammonite::scale(aVec, bVec[0], cVec);
    for (std::size_t i = 0; i < size; i++) {
      if ((T)(aVec[i] * bVec[0]) != cVec[i]) {
        reportFailure("Vector scaling failed", aVec, bVec, cVec);
        return false;
      }
    }

    //Test in-place scaling
    ammonite::copy(aVec, cVec);
    ammonite::scale(cVec, bVec[0]);
    for (std::size_t i = 0; i < size; i++) {
      if ((T)(aVec[i] * bVec[0]) != cVec[i]) {
        reportFailure("In-place vector scaling failed", aVec, bVec, cVec);
        return false;
      }
    }

    return true;
  }

  template <typename T, std::size_t size> requires ammonite::validVector<T, size>
  bool testDiv() {
    ammonite::Vec<T, size> aVec = {0};
    ammonite::Vec<T, size> bVec = {0};
    ammonite::Vec<T, size> cVec = {0};
    randomFillVector(aVec);
    randomFillVector(bVec);

    //Avoid division by zero
    if (bVec[0] == (T)0) {
      bVec[0]++;
    }

    //Test regular division
    ammonite::div(aVec, bVec[0], cVec);
    for (std::size_t i = 0; i < size; i++) {
      if ((T)(aVec[i] / bVec[0]) != cVec[i]) {
        reportFailure("Vector division failed", aVec, bVec, cVec);
        return false;
      }
    }

    //Test in-place division
    ammonite::copy(aVec, cVec);
    ammonite::div(cVec, bVec[0]);
    for (std::size_t i = 0; i < size; i++) {
      if ((T)(aVec[i] / bVec[0]) != cVec[i]) {
        reportFailure("In-place vector division failed", aVec, bVec, cVec);
        return false;
      }
    }

    return true;
  }

  template <typename T, std::size_t size> requires ammonite::validVector<T, size>
  bool testNormalise() {
    ammonite::Vec<T, size> aVec = {0};
    ammonite::Vec<T, size> bVec = {0};
    randomFillVector(aVec);

    //Adjust all-zero vectors
    bool allZero = true;
    for (std::size_t i = 0; i < size; i++) {
      if (aVec[i] != (T)0) {
        allZero = false;
        break;
      }
    }

    if (allZero) {
      aVec[0]++;
    }

    T sum = (T)0;
    for (std::size_t i = 0; i < size; i++) {
      sum += aVec[i] * aVec[i];
    }
    T length = (T)std::sqrt(sum);

    //Skip (effectively) zero length vectors
    if (length == 0) {
      return true;
    }

    //Test regular normalisation
    ammonite::normalise(aVec, bVec);
    for (std::size_t i = 0; i < size; i++) {
      if ((T)(aVec[i] / length) != bVec[i]) {
        reportFailure("Vector normalisation failed", aVec, bVec);
        return false;
      }
    }

    //Test in-place normalisation
    ammonite::copy(aVec, bVec);
    ammonite::normalise(bVec);
    for (std::size_t i = 0; i < size; i++) {
      if ((T)(aVec[i] / length) != bVec[i]) {
        reportFailure("In-place vector normalisation failed", aVec, bVec);
        return false;
      }
    }

    return true;
  }

  template <typename T, std::size_t size> requires ammonite::validVector<T, size>
  bool testDot() {
    ammonite::Vec<T, size> aVec = {0};
    ammonite::Vec<T, size> bVec = {0};
    randomFillVector(aVec);
    randomFillVector(bVec);

    T sum = (T)0;
    for (std::size_t i = 0; i < size; i++) {
      sum += aVec[i] * bVec[i];
    }

    //Test dot product
    if (ammonite::dot(aVec, bVec) != sum) {
      reportFailure("Vector dot product failed", aVec, bVec);
      return false;
    }

    return true;
  }

  template <typename T, std::size_t size> requires ammonite::validVector<T, size>
  bool testCross() {
    if constexpr (size == 3) {
      ammonite::Vec<T, 3> aVec = {0};
      ammonite::Vec<T, 3> bVec = {0};
      ammonite::Vec<T, 3> cVec = {0};
      randomFillVector(aVec);
      randomFillVector(bVec);

      //Test cross product
      ammonite::cross(aVec, bVec, cVec);
      for (std::size_t i = 0; i < 3; i++) {
        const std::size_t oneOffset = (i + 1) % 3;
        const std::size_t twoOffset = (i + 2) % 3;
        T component = (aVec[oneOffset] * bVec[twoOffset]) - (aVec[twoOffset] * bVec[oneOffset]);
        if (cVec[i] != component) {
          reportFailure("Vector cross product failed", aVec, bVec, cVec);
          return false;
        }
      }
    }

    return true;
  }

  template <typename T, std::size_t size> requires ammonite::validVector<T, size>
  bool testLength() {
    ammonite::Vec<T, size> aVec = {0};
    randomFillVector(aVec);

    T sum = (T)0;
    for (std::size_t i = 0; i < size; i++) {
      sum += aVec[i] * aVec[i];
    }
    T length = (T)std::sqrt(sum);

    //Test vector length
    if (ammonite::length(aVec) != length) {
      reportFailure("Vector length failed", aVec, length);
      return false;
    }

    return true;
  }

  template <typename T, std::size_t size> requires ammonite::validVector<T, size>
  bool testDistance() {
    ammonite::Vec<T, size> aVec = {0};
    ammonite::Vec<T, size> bVec = {0};
    randomFillVector(aVec);
    randomFillVector(bVec);

    //Swap elements that would cause a negative for promoted types
    if constexpr (std::is_same_v<T, unsigned char> || std::is_same_v<T, unsigned short>) {
      for (std::size_t i = 0; i < size; i++) {
        if (aVec[i] > bVec[i]) {
          T temp = aVec[i];
          aVec[i] = bVec[i];
          bVec[i] = temp;
        }
      }
    }

    T sum = (T)0;
    for (std::size_t i = 0; i < size; i++) {
      T diff = bVec[i] - aVec[i];
      sum += (T)(diff * diff);
    }
    T distance = (T)std::sqrt(sum);

    //Test vector distance
    if (ammonite::distance(aVec, bVec) != distance) {
      reportFailure("Vector distance failed", aVec, bVec);
      return false;
    }

    return true;
  }

  template <typename T, std::size_t size> requires ammonite::validVector<T, size>
  bool testVectors(std::string_view typeName) {
    ammonite::utils::normal << "Testing " << size << "x " << typeName << " vectors" << std::endl;

    //Test NamedVec
    if (!testNamedVec<T, size>()) {
      return false;
    }

    for (int i = 0; i < 10000; i++) {
      //Test ammonite::equal()
      if (!testEqual<T, size>()) {
        return false;
      }

      //Test ammonite::copy()
      if (!testCopy<T, size>()) {
        return false;
      }

      //Test ammonite::copyCast()
      if (!testCopyCast<T, size>()) {
        return false;
      }

      //Test ammonite::add()
      if (!testAdd<T, size>()) {
        return false;
      }

      //Test ammonite::sub()
      if (!testSub<T, size>()) {
        return false;
      }

      //Test ammonite::scale()
      if (!testScale<T, size>()) {
        return false;
      }

      //Test ammonite::div()
      if (!testDiv<T, size>()) {
        return false;
      }

      //Test ammonite::normalise()
      if (!testNormalise<T, size>()) {
        return false;
      }

      //Test ammonite::dot()
      if (!testDot<T, size>()) {
        return false;
      }

      //Test ammonite::cross()
      if (!testCross<T, size>()) {
        return false;
      }

      //Test ammonite::length()
      if (!testLength<T, size>()) {
        return false;
      }

      //Test ammonite::distance()
      if (!testDistance<T, size>()) {
        return false;
      }
    }

    return true;
  }
}

namespace tests {
  template <typename T, std::size_t size> requires ammonite::validVector<T, size>
  bool testVectors(std::string_view typeName) {
    ammonite::utils::normal << "Testing " << size << "x " << typeName << " vectors" << std::endl;

    //Test NamedVec
    if (!testNamedVec<T, size>()) {
      return false;
    }

    for (int i = 0; i < 10000; i++) {
      //Test ammonite::equal()
      if (!testEqual<T, size>()) {
        return false;
      }

      //Test ammonite::copy()
      if (!testCopy<T, size>()) {
        return false;
      }

      //Test ammonite::copyCast()
      if (!testCopyCast<T, size>()) {
        return false;
      }

      //Test ammonite::add()
      if (!testAdd<T, size>()) {
        return false;
      }

      //Test ammonite::sub()
      if (!testSub<T, size>()) {
        return false;
      }

      //Test ammonite::scale()
      if (!testScale<T, size>()) {
        return false;
      }

      //Test ammonite::div()
      if (!testDiv<T, size>()) {
        return false;
      }

      //Test ammonite::normalise()
      if (!testNormalise<T, size>()) {
        return false;
      }

      //Test ammonite::dot()
      if (!testDot<T, size>()) {
        return false;
      }

      //Test ammonite::cross()
      if (!testCross<T, size>()) {
        return false;
      }

      //Test ammonite::length()
      if (!testLength<T, size>()) {
        return false;
      }

      //Test ammonite::distance()
      if (!testDistance<T, size>()) {
        return false;
      }
    }

    return true;
  }
}

namespace tests {
  bool testInt8x2();
  bool testInt16x2();
  bool testInt32x2();
  bool testInt64x2();
  bool testUInt8x2();
  bool testUInt16x2();
  bool testUInt32x2();
  bool testUInt64x2();
  bool testFloatx2();
  bool testDoublex2();

  bool testInt8x3();
  bool testInt16x3();
  bool testInt32x3();
  bool testInt64x3();
  bool testUInt8x3();
  bool testUInt16x3();
  bool testUInt32x3();
  bool testUInt64x3();
  bool testFloatx3();
  bool testDoublex3();

  bool testInt8x4();
  bool testInt16x4();
  bool testInt32x4();
  bool testInt64x4();
  bool testUInt8x4();
  bool testUInt16x4();
  bool testUInt32x4();
  bool testUInt64x4();
  bool testFloatx4();
  bool testDoublex4();
}

#endif
