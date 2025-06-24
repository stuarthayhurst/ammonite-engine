#ifndef MATRIXTESTTEMPLATES
#define MATRIXTESTTEMPLATES

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string_view>
#include <type_traits>

#include <ammonite/ammonite.hpp>

//Test helpers
namespace {
  template <typename T, std::size_t cols, std::size_t rows>
            requires ammonite::validMatrix<T, cols, rows>
  void randomFillMatrix(ammonite::Mat<T, cols, rows>& mat) {
    for (std::size_t col = 0; col < cols; col++) {
      for (std::size_t row = 0; row < rows; row++) {
        mat[col][row] = (T)ammonite::utils::randomDouble(1000000.0f);
      }
    }
  }

  template <typename T, std::size_t cols, std::size_t rows>
            requires ammonite::validMatrix<T, cols, rows>
  bool roughly(T a, T b) {
    if constexpr (std::is_integral_v<T>) {
      return (a == b);
    } else {
      return std::abs(a - b) <= std::max((T)1e-5f, std::max(a, b) * (T)0.001);
    }
  }
}

//Tests
namespace {
  template <typename T, std::size_t cols, std::size_t rows>
            requires ammonite::validMatrix<T, cols, rows>
  bool testEqual() {
    ammonite::Mat<T, cols, rows> aMat = {{0}};
    ammonite::Mat<T, cols, rows> bMat = {{0}};
    randomFillMatrix(aMat);

    //Set bMat to aMat
    for (std::size_t col = 0; col < cols; col++) {
      for (std::size_t row = 0; row < rows; row++) {
        bMat[col][row] = aMat[col][row];
      }
    }

    //Check ammonite::equal() on equal matrices
    if (!ammonite::equal(aMat, bMat)) {
      ammonite::utils::error << "Equal matrix comparison failed" << std::endl;
      ammonite::utils::normal << "  Input:\n" << ammonite::formatMatrix(aMat) \
                              << "\n  Input:\n" << ammonite::formatMatrix(bMat) \
                              << std::endl;
      return false;
    }

    //Safely guarantee a modification to bMat
    uintmax_t tmp = 0;
    std::memcpy(&tmp, &bMat[0][0], sizeof(bMat[0][0]));
    tmp ^= 1;
    std::memcpy(&bMat[0][0], &tmp, sizeof(bMat[0][0]));

    //Check ammonite::equal() on unequal vectors
    if (ammonite::equal(aMat, bMat)) {
      ammonite::utils::error << "Unequal matrix comparison failed" << std::endl;
      ammonite::utils::normal << "  Input:\n" << ammonite::formatMatrix(aMat) \
                              << "\n  Input:\n" << ammonite::formatMatrix(bMat) \
                              << std::endl;
      return false;
    }

    return true;
  }

  template <typename T, std::size_t cols, std::size_t rows>
            requires ammonite::validMatrix<T, cols, rows>
  bool testCopy() {
    //TODO
    return true;
  }

  template <typename T, std::size_t cols, std::size_t rows>
            requires ammonite::validMatrix<T, cols, rows>
  bool testCopyCast() {
    //TODO
    return true;
  }
}

namespace tests {
  template <typename T, std::size_t cols, std::size_t rows>
            requires ammonite::validMatrix<T, cols, rows>
  bool testMatrix(std::string_view typeName) {
    ammonite::utils::normal << "Testing " << cols << "x" << rows << " " \
                            << typeName << " matrices" << std::endl;

    for (int i = 0; i < 10000; i++) {
      //Test ammonite::equal()
      if (!testEqual<T, cols, rows>()) {
        return false;
      }

      //Test ammonite::copy()
      if (!testCopy<T, cols, rows>()) {
        return false;
      }

      //Test ammonite::copyCast()
      if (!testCopyCast<T, cols, rows>()) {
        return false;
      }
    }

    return true;
  }
}

namespace tests {
  bool testFloatx2x2();
  bool testFloatx2x3();
  bool testFloatx2x4();
  bool testDoublex2x2();
  bool testDoublex2x3();
  bool testDoublex2x4();

  bool testFloatx3x2();
  bool testFloatx3x3();
  bool testFloatx3x4();
  bool testDoublex3x2();
  bool testDoublex3x3();
  bool testDoublex3x4();

  bool testFloatx4x2();
  bool testFloatx4x3();
  bool testFloatx4x4();
  bool testDoublex4x2();
  bool testDoublex4x3();
  bool testDoublex4x4();
}

#endif
