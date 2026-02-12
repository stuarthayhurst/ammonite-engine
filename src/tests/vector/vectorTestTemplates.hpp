#ifndef VECTORTESTTEMPLATES
#define VECTORTESTTEMPLATES

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string_view>
#include <type_traits>

#include <ammonite/ammonite.hpp>

#include "../support.hpp"

namespace {
  template <typename T, unsigned int size> requires ammonite::validVector<T, size>
  bool testNamedVec() {
    ammonite::Vec<T, size> aVec = {0};
    ammonite::NamedVec<T, size> namedAVec(aVec);

    if (&namedAVec.x != &aVec[0]) {
      ammonite::utils::error << "Named vector has a different address to its underlying vector" << std::endl;
      ammonite::utils::normal << "  Result:   " << (void*)&namedAVec.x \
                              << "\n  Expected: " << (void*)&aVec[0] << std::endl;
      return false;
    }

    return true;
  }

  template <typename T, unsigned int size> requires ammonite::validVector<T, size>
  bool testData() {
    ammonite::Vec<T, size> aVec = {0};

    if ((void*)ammonite::data(aVec) != (void*)&aVec) {
      ammonite::utils::error << "Data pointer has a different address to the vector" << std::endl;
      ammonite::utils::normal << "  Result:   " << (void*)ammonite::data(aVec) \
                              << "\n  Expected: " << (void*)&aVec << std::endl;
      return false;
    }

    return true;
  }

  template <typename T, unsigned int size> requires ammonite::validVector<T, size>
  bool testEqual() {
    ammonite::Vec<T, size> aVec = {0};
    ammonite::Vec<T, size> bVec = {0};
    randomFillVector(aVec);

    //Set bVec to aVec
    for (unsigned int i = 0; i < size; i++) {
      bVec[i] = aVec[i];
    }

    //Check ammonite::equal() on equal vectors
    if (!ammonite::equal(aVec, bVec)) {
      ammonite::utils::error << "Equal vector comparison failed" << std::endl;
      ammonite::utils::normal << "  Input: " << ammonite::formatVector(aVec) \
                              << "\n  Input: " << ammonite::formatVector(bVec) \
                              << std::endl;
      return false;
    }

    for (unsigned int i = 0; i < size; i++) {
      //Safely guarantee a modification to bVec
      uintmax_t tmp = 0;
      std::memcpy(&tmp, &bVec[i], sizeof(bVec[i]));
      tmp ^= 1;
      std::memcpy(&bVec[i], &tmp, sizeof(bVec[i]));

      //Check ammonite::equal() on unequal vectors
      if (ammonite::equal(aVec, bVec)) {
        ammonite::utils::error << "Unequal vector comparison failed" << std::endl;
        ammonite::utils::normal << "  Input: " << ammonite::formatVector(aVec) \
                                << "\n  Input: " << ammonite::formatVector(bVec) \
                                << std::endl;
        return false;
      }

      //Revert the change
      bVec[i] = aVec[i];
    }

    return true;
  }

  template <typename T, unsigned int size> requires ammonite::validVector<T, size>
  bool testCopy() {
    ammonite::Vec<T, size> aVec = {0};
    ammonite::Vec<T, size> bVec = {0};
    randomFillVector(aVec);

    ammonite::copy(aVec, bVec);
    if (!ammonite::equal(aVec, bVec)) {
      ammonite::utils::error << "Vector copy failed" << std::endl;
      ammonite::utils::normal << "  Result:   " << ammonite::formatVector(bVec) \
                              << "\n  Expected: " << ammonite::formatVector(aVec) \
                              << std::endl;
      return false;
    }

    //Check vectors are fully preserved when copying to a max size vector
    ammonite::Vec<T, 4> cVec = {0};
    ammonite::copy(aVec, bVec);
    ammonite::copy(aVec, cVec);
    ammonite::copy(cVec, aVec);
    if (!ammonite::equal(aVec, bVec)) {
      ammonite::utils::error << "Vector grow copy failed" << std::endl;
      ammonite::utils::normal << "  Result:   " << ammonite::formatVector(aVec) \
                              << "\n  Expected: " << ammonite::formatVector(bVec) \
                              << std::endl;
      return false;
    }

    //Check relevant parts are preserved when copying to a min size vector
    ammonite::Vec<T, 2> dVec = {0};
    ammonite::copy(aVec, dVec);
    if (aVec[0] != dVec[0] || aVec[1] != dVec[1]) {
      ammonite::utils::error << "Vector shrink copy failed" << std::endl;
      ammonite::utils::normal << "  Result:   " << ammonite::formatVector(dVec) \
                              << "\n  Expected: " << ammonite::formatVector(aVec)
                              << std::endl;
      return false;
    }

    return true;
  }

  template <typename T, unsigned int size> requires ammonite::validVector<T, size>
  bool testCopyCast() {
    ammonite::Vec<T, size> aVec = {0};
    ammonite::Vec<double, size> bVec = {0};
    randomFillVector(aVec);

    ammonite::copyCast(aVec, bVec);
    for (unsigned int i = 0; i < size; i++) {
      if ((double)aVec[i] != bVec[i]) {
        ammonite::utils::error << "Vector copy cast failed" << std::endl;
        ammonite::utils::normal << "  Result:   " << ammonite::formatVector(bVec) \
                                << "\n  Expected: " << ammonite::formatVector(aVec)
                                << std::endl;
        return false;
      }
    }

    //Check vectors are fully preserved when copying to a max size vector
    ammonite::Vec<double, 4> cVec = {0};
    ammonite::copyCast(aVec, cVec);
    for (unsigned int i = 0; i < size; i++) {
      if ((double)aVec[i] != cVec[i]) {
        ammonite::utils::error << "Vector grow copy cast failed" << std::endl;
        ammonite::utils::normal << "  Result:   " << ammonite::formatVector(cVec) \
                                << "\n  Expected: " << ammonite::formatVector(aVec)
                                << std::endl;
        return false;
      }
    }

    //Check relevant parts are preserved when copying to a min size vector
    ammonite::Vec<double, 2> dVec = {0};
    ammonite::copyCast(aVec, dVec);
    if ((double)aVec[0] != dVec[0] || (double)aVec[1] != dVec[1]) {
      ammonite::utils::error << "Vector shrink copy cast failed" << std::endl;
      ammonite::utils::normal << "  Result:   " << ammonite::formatVector(dVec) \
                              << "\n  Expected: " << ammonite::formatVector(aVec)
                              << std::endl;
      return false;
    }

    return true;
  }

  template <typename T, unsigned int size> requires ammonite::validVector<T, size>
  bool testSet() {
    ammonite::Vec<T, size> aVec = {0};
    randomFillVector(aVec);
    T b = randomScalar<T>();

    //Test vector scalar initialisation
    ammonite::set(aVec, b);
    for (unsigned int i = 0; i < size; i++) {
      if (aVec[i] != b) {
        ammonite::utils::error << "Vector set failed" << std::endl;
        ammonite::utils::normal << "  Result:   " << ammonite::formatVector(aVec) \
                                << "\n  Expected: " << b \
                                << " at index " << i << std::endl;
        return false;
      }
    }

    //Test vector vector-scalar initialisation
    if constexpr (size >= 3) {
      ammonite::Vec<T, size - 1> bVec = {0};
      randomFillVector(bVec);

      ammonite::set(aVec, bVec, b);
      for (unsigned int i = 0; i < size - 1; i++) {
        if (aVec[i] != bVec[i]) {
          ammonite::utils::error << "Vector-scalar set failed" << std::endl;
          ammonite::utils::normal << "  Result:   " << ammonite::formatVector(aVec) \
                                  << "\n  Expected: " << bVec[i] \
                                  << " at index " << i << std::endl;
          return false;
        }
      }

      if (aVec[size - 1] != b) {
        ammonite::utils::error << "Individual scalar set failed" << std::endl;
        ammonite::utils::normal << "  Result:   " << ammonite::formatVector(aVec) \
                                << "\n  Expected: " << b \
                                << " at index " << size - 1 << std::endl;
        return false;
      }
    }

    //Test full, individual vector initialisation
    ammonite::Vec<T, size> cVec = {0};
    randomFillVector(cVec);
    if constexpr (size == 2) {
      ammonite::set(aVec, cVec[0], cVec[1]);
    } else if constexpr (size == 3) {
      ammonite::set(aVec, cVec[0], cVec[1], cVec[2]);
    } else if constexpr (size == 4) {
      ammonite::set(aVec, cVec[0], cVec[1], cVec[2], cVec[3]);
    }

    for (unsigned int i = 0; i < size - 1; i++) {
      if (aVec[i] != cVec[i]) {
        ammonite::utils::error << "Vector set failed" << std::endl;
        ammonite::utils::normal << "  Result:   " << ammonite::formatVector(aVec) \
                                << "\n  Expected: " << ammonite::formatVector(cVec) \
                                << std::endl;
        return false;
      }
    }

    return true;
  }

  template <typename T, unsigned int size> requires ammonite::validVector<T, size>
  bool testAdd() {
    ammonite::Vec<T, size> aVec = {0};
    ammonite::Vec<T, size> bVec = {0};
    ammonite::Vec<T, size> cVec = {0};
    T d = randomScalar<T>();
    randomFillVector(aVec);
    randomFillVector(bVec);

    //Test regular addition
    ammonite::add(aVec, bVec, cVec);
    for (unsigned int i = 0; i < size; i++) {
      if ((T)(aVec[i] + bVec[i]) != cVec[i]) {
        ammonite::utils::error << "Vector addition failed" << std::endl;
        ammonite::utils::normal << "  Input:  " << ammonite::formatVector(aVec) \
                                << "\n  Input:  " << ammonite::formatVector(bVec) \
                                << "\n  Result: " << ammonite::formatVector(cVec) \
                                << std::endl;
        return false;
      }
    }

    //Test in-place addition
    ammonite::copy(aVec, cVec);
    ammonite::add(cVec, bVec);
    for (unsigned int i = 0; i < size; i++) {
      if ((T)(aVec[i] + bVec[i]) != cVec[i]) {
        ammonite::utils::error << "In-place vector addition failed" << std::endl;
        ammonite::utils::normal << "  Input:  " << ammonite::formatVector(aVec) \
                                << "\n  Input:  " << ammonite::formatVector(bVec) \
                                << "\n  Result: " << ammonite::formatVector(cVec) \
                                << std::endl;
        return false;
      }
    }

    //Test scalar addition
    ammonite::add(aVec, d, cVec);
    for (unsigned int i = 0; i < size; i++) {
      if ((T)(aVec[i] + d) != cVec[i]) {
        ammonite::utils::error << "Scalar vector addition failed" << std::endl;
        ammonite::utils::normal << "  Input:  " << ammonite::formatVector(aVec) \
                                << "\n  Input:  " << d \
                                << "\n  Result: " << ammonite::formatVector(cVec) \
                                << std::endl;
        return false;
      }
    }

    //Test in-place scalar addition
    ammonite::copy(aVec, cVec);
    ammonite::add(cVec, d);
    for (unsigned int i = 0; i < size; i++) {
      if ((T)(aVec[i] + d) != cVec[i]) {
        ammonite::utils::error << "In-place scalar vector addition failed" << std::endl;
        ammonite::utils::normal << "  Input:  " << ammonite::formatVector(aVec) \
                                << "\n  Input:  " << d \
                                << "\n  Result: " << ammonite::formatVector(cVec) \
                                << std::endl;
        return false;
      }
    }

    return true;
  }

  template <typename T, unsigned int size> requires ammonite::validVector<T, size>
  bool testSub() {
    ammonite::Vec<T, size> aVec = {0};
    ammonite::Vec<T, size> bVec = {0};
    ammonite::Vec<T, size> cVec = {0};
    T d = randomScalar<T>();
    randomFillVector(aVec);
    randomFillVector(bVec);

    //Test regular subtraction
    ammonite::sub(aVec, bVec, cVec);
    for (unsigned int i = 0; i < size; i++) {
      if ((T)(aVec[i] - bVec[i]) != cVec[i]) {
        ammonite::utils::error << "Vector subtraction failed" << std::endl;
        ammonite::utils::normal << "  Input:  " << ammonite::formatVector(aVec) \
                                << "\n  Input:  " << ammonite::formatVector(bVec) \
                                << "\n  Result: " << ammonite::formatVector(cVec) \
                                << std::endl;
        return false;
      }
    }

    //Test in-place subtraction
    ammonite::copy(aVec, cVec);
    ammonite::sub(cVec, bVec);
    for (unsigned int i = 0; i < size; i++) {
      if ((T)(aVec[i] - bVec[i]) != cVec[i]) {
        ammonite::utils::error << "In-place vector subtraction failed" << std::endl;
        ammonite::utils::normal << "  Input:  " << ammonite::formatVector(aVec) \
                                << "\n  Input:  " << ammonite::formatVector(bVec) \
                                << "\n  Result: " << ammonite::formatVector(cVec) \
                                << std::endl;
        return false;
      }
    }

    //Test scalar subtraction
    ammonite::sub(aVec, d, cVec);
    for (unsigned int i = 0; i < size; i++) {
      if ((T)(aVec[i] - d) != cVec[i]) {
        ammonite::utils::error << "Scalar vector subtraction failed" << std::endl;
        ammonite::utils::normal << "  Input:  " << ammonite::formatVector(aVec) \
                                << "\n  Input:  " << d \
                                << "\n  Result: " << ammonite::formatVector(cVec) \
                                << std::endl;
        return false;
      }
    }

    //Test in-place scalar subtraction
    ammonite::copy(aVec, cVec);
    ammonite::sub(cVec, d);
    for (unsigned int i = 0; i < size; i++) {
      if ((T)(aVec[i] - d) != cVec[i]) {
        ammonite::utils::error << "In-place scalar vector subtraction failed" << std::endl;
        ammonite::utils::normal << "  Input:  " << ammonite::formatVector(aVec) \
                                << "\n  Input:  " << d \
                                << "\n  Result: " << ammonite::formatVector(cVec) \
                                << std::endl;
        return false;
      }
    }

    return true;
  }

  template <typename T, unsigned int size> requires ammonite::validVector<T, size>
  bool testScale() {
    ammonite::Vec<T, size> aVec = {0};
    ammonite::Vec<T, size> bVec = {0};
    ammonite::Vec<T, size> cVec = {0};
    T d = randomScalar<T>();
    randomFillVector(aVec);
    randomFillVector(bVec);

    //Test regular scaling
    ammonite::scale(aVec, d, cVec);
    for (unsigned int i = 0; i < size; i++) {
      if ((T)(aVec[i] * d) != cVec[i]) {
        ammonite::utils::error << "Vector scaling failed" << std::endl;
        ammonite::utils::normal << "  Input:  " << ammonite::formatVector(aVec) \
                                << "\n  Input:  " << d \
                                << "\n  Result: " << ammonite::formatVector(cVec) \
                                << std::endl;
        return false;
      }
    }

    //Test in-place scaling
    ammonite::copy(aVec, cVec);
    ammonite::scale(cVec, d);
    for (unsigned int i = 0; i < size; i++) {
      if ((T)(aVec[i] * d) != cVec[i]) {
        ammonite::utils::error << "In-place vector scaling failed" << std::endl;
        ammonite::utils::normal << "  Input:  " << ammonite::formatVector(aVec) \
                                << "\n  Input:  " << d \
                                << "\n  Result: " << ammonite::formatVector(cVec) \
                                << std::endl;
        return false;
      }
    }

    return true;
  }

  template <typename T, unsigned int size> requires ammonite::validVector<T, size>
  bool testDivide() {
    ammonite::Vec<T, size> aVec = {0};
    ammonite::Vec<T, size> bVec = {0};
    T c = randomScalar<T>();
    randomFillVector(aVec);

    //Avoid division by zero
    if (c == (T)0) {
      c++;
    }

    //Test regular division
    ammonite::divide(aVec, c, bVec);
    for (unsigned int i = 0; i < size; i++) {
      if ((T)(aVec[i] / c) != bVec[i]) {
        ammonite::utils::error << "Vector division failed" << std::endl;
        ammonite::utils::normal << "  Input:  " << ammonite::formatVector(aVec) \
                                << "\n  Input:  " << c \
                                << "\n  Result: " << ammonite::formatVector(bVec) \
                                << std::endl;
        return false;
      }
    }

    //Test in-place division
    ammonite::copy(aVec, bVec);
    ammonite::divide(bVec, c);
    for (unsigned int i = 0; i < size; i++) {
      if ((T)(aVec[i] / c) != bVec[i]) {
        ammonite::utils::error << "In-place vector division failed" << std::endl;
        ammonite::utils::normal << "  Input:  " << ammonite::formatVector(aVec) \
                                << "\n  Input:  " << c \
                                << "\n  Result: " << ammonite::formatVector(bVec) \
                                << std::endl;
        return false;
      }
    }

    return true;
  }

  template <typename T, unsigned int size> requires ammonite::validVector<T, size>
  bool testNegate() {
    if constexpr (std::is_signed_v<T>) {
      ammonite::Vec<T, size> aVec = {0};
      ammonite::Vec<T, size> bVec = {0};
      randomFillVector(aVec);

      //Test regular negation
      ammonite::negate(aVec, bVec);
      for (unsigned int i = 0; i < size; i++) {
        if (aVec[i] != -bVec[i]) {
          ammonite::utils::error << "Vector negation failed" << std::endl;
          ammonite::utils::normal << "  Input:  " << ammonite::formatVector(aVec) \
                                  << "\n  Result: " << ammonite::formatVector(bVec) \
                                  << std::endl;
          return false;
        }
      }

      //Test in-place negtation
      ammonite::copy(aVec, bVec);
      ammonite::negate(aVec);
      for (unsigned int i = 0; i < size; i++) {
        if (aVec[i] != -bVec[i]) {
          ammonite::utils::error << "In-place vector negation failed" << std::endl;
          ammonite::utils::normal << "  Input:  " << ammonite::formatVector(bVec) \
                                  << "\n  Result: " << ammonite::formatVector(aVec) \
                                  << std::endl;
          return false;
        }
      }
    }

    return true;
  }

  template <typename T, unsigned int size> requires ammonite::validVector<T, size>
  bool testLength() {
    ammonite::Vec<T, size> aVec = {0};
    randomFillVector(aVec);

    T sum = (T)0;
    for (unsigned int i = 0; i < size; i++) {
      sum += aVec[i] * aVec[i];
    }
    T length = (T)std::sqrt(sum);

    //Test vector length
    if (!roughly(ammonite::length(aVec), length)) {
      ammonite::utils::error << "Vector length calculation failed" << std::endl;
      ammonite::utils::normal << "  Input:    " << ammonite::formatVector(aVec) \
                              << "\n  Result:   " << ammonite::length(aVec) \
                              << "\n  Expected: " << length << std::endl;

      return false;
    }

    return true;
  }

  template <typename T, unsigned int size> requires ammonite::validVector<T, size>
  bool testNormalise() {
    ammonite::Vec<T, size> aVec = {0};
    ammonite::Vec<T, size> bVec = {0};
    randomFillVector(aVec);

    //Skip (effectively) zero length vectors
    T length = ammonite::length(aVec);
    if (length == 0) {
      return true;
    }

    //Test regular normalisation
    ammonite::normalise(aVec, bVec);
    for (unsigned int i = 0; i < size; i++) {
      if (!roughly((T)(aVec[i] / length), bVec[i])) {
        ammonite::utils::error << "Vector normalisation failed" << std::endl;
        ammonite::utils::normal << "  Input:    " << ammonite::formatVector(aVec) \
                                << "\n  Result:   " << ammonite::formatVector(bVec) \
                                << "\n  Expected: " << (T)(aVec[i] / length) \
                                << " at index " << i << std::endl;
        return false;
      }
    }

    //Test in-place normalisation
    ammonite::copy(aVec, bVec);
    ammonite::normalise(bVec);
    for (unsigned int i = 0; i < size; i++) {
      if (!roughly((T)(aVec[i] / length), bVec[i])) {
        ammonite::utils::error << "In-place vector normalisation failed" << std::endl;
        ammonite::utils::normal << "  Input:    " << ammonite::formatVector(aVec) \
                                << "\n  Result:   " << ammonite::formatVector(bVec) \
                                << "\n  Expected: " << (T)(aVec[i] / length) \
                                << " at index " << i << std::endl;
        return false;
      }
    }

    return true;
  }

  template <typename T, unsigned int size> requires ammonite::validVector<T, size>
  bool testDot() {
    ammonite::Vec<T, size> aVec = {0};
    ammonite::Vec<T, size> bVec = {0};
    randomFillVector(aVec);
    randomFillVector(bVec);

    T sum = (T)0;
    for (unsigned int i = 0; i < size; i++) {
      sum += aVec[i] * bVec[i];
    }

    //Test dot product
    if (!roughly(ammonite::dot(aVec, bVec), sum)) {
      ammonite::utils::error << "Vector dot product failed" << std::endl;
      ammonite::utils::normal << "  Input:    " << ammonite::formatVector(aVec) \
                              << "\n  Input:    " << ammonite::formatVector(bVec) \
                              << "\n  Result:   " << ammonite::dot(aVec, bVec) \
                              << "\n  Expected: " << sum << std::endl;
      return false;
    }

    return true;
  }

  template <typename T, unsigned int size> requires ammonite::validVector<T, size>
  bool testCross() {
    if constexpr (size == 3) {
      ammonite::Vec<T, 3> aVec = {0};
      ammonite::Vec<T, 3> bVec = {0};
      ammonite::Vec<T, 3> cVec = {0};
      randomFillVector(aVec);
      randomFillVector(bVec);

      //Test cross product
      ammonite::cross(aVec, bVec, cVec);
      for (unsigned int i = 0; i < 3; i++) {
        const unsigned int oneOffset = (i + 1) % 3;
        const unsigned int twoOffset = (i + 2) % 3;
        T component = (aVec[oneOffset] * bVec[twoOffset]) - (aVec[twoOffset] * bVec[oneOffset]);
        if (cVec[i] != component) {
          ammonite::utils::error << "Vector cross product failed" << std::endl;
          ammonite::utils::normal << "  Input:    " << ammonite::formatVector(aVec) \
                                  << "\n  Input:    " << ammonite::formatVector(bVec) \
                                  << "\n  Result:   " << ammonite::formatVector(cVec) \
                                  << "\n  Expected: " << component \
                                  << " at index " << i << std::endl;

          return false;
        }
      }
    }

    return true;
  }

  template <typename T, unsigned int size> requires ammonite::validVector<T, size>
  bool testDistance() {
    ammonite::Vec<T, size> aVec = {0};
    ammonite::Vec<T, size> bVec = {0};
    randomFillVector(aVec);
    randomFillVector(bVec);

    //Swap elements that would cause a negative for promoted types
    if constexpr (std::is_same_v<T, unsigned char> || std::is_same_v<T, unsigned short>) {
      for (unsigned int i = 0; i < size; i++) {
        if (aVec[i] > bVec[i]) {
          T temp = aVec[i];
          aVec[i] = bVec[i];
          bVec[i] = temp;
        }
      }
    }

    T sum = (T)0;
    for (unsigned int i = 0; i < size; i++) {
      T diff = bVec[i] - aVec[i];
      sum += (T)(diff * diff);
    }
    T distance = (T)std::sqrt(sum);

    //Test vector distance
    if (!roughly(ammonite::distance(aVec, bVec), distance)) {
      ammonite::utils::error << "Vector distance failed" << std::endl;
      ammonite::utils::normal << "  Input:    " << ammonite::formatVector(aVec) \
                              << "\n  Input:    " << ammonite::formatVector(bVec) \
                              << "\n  Result:   " << ammonite::distance(aVec, bVec) \
                              << "\n  Expected: " << distance << std::endl;
      return false;
    }

    return true;
  }
}

namespace tests {
  template <typename T, unsigned int size> requires ammonite::validVector<T, size>
  bool testVector(std::string_view typeName) {
    ammonite::utils::normal << "Testing " << size << "x " << typeName << " vectors" << std::endl;

    //Test NamedVec
    if (!testNamedVec<T, size>()) {
      return false;
    }

    //Test ammonite::data()
    if (!testData<T, size>()) {
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

      //Test ammonite::set()
      if (!testSet<T, size>()) {
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

      //Test ammonite::divide()
      if (!testDivide<T, size>()) {
        return false;
      }

      //Test ammonite::negate()
      if (!testNegate<T, size>()) {
        return false;
      }

      //Test ammonite::length()
      if (!testLength<T, size>()) {
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

      //Test ammonite::distance()
      if (!testDistance<T, size>()) {
        return false;
      }
    }

    return true;
  }
}

namespace tests {
  bool testInt32x2();
  bool testInt64x2();
  bool testUInt32x2();
  bool testUInt64x2();
  bool testFloatx2();
  bool testDoublex2();

  bool testInt32x3();
  bool testInt64x3();
  bool testUInt32x3();
  bool testUInt64x3();
  bool testFloatx3();
  bool testDoublex3();

  bool testInt32x4();
  bool testInt64x4();
  bool testUInt32x4();
  bool testUInt64x4();
  bool testFloatx4();
  bool testDoublex4();
}

#endif
