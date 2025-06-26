#ifndef MATRIXTESTTEMPLATES
#define MATRIXTESTTEMPLATES

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string_view>
#include <type_traits>

#include <ammonite/ammonite.hpp>

#include "../vector/vectorTestTemplates.hpp"

//Test helpers
namespace {
  template <typename T, unsigned int cols, unsigned int rows>
            requires ammonite::validMatrix<T, cols, rows>
  void randomFillMatrix(ammonite::Mat<T, cols, rows>& mat) {
    for (unsigned int col = 0; col < cols; col++) {
      for (unsigned int row = 0; row < rows; row++) {
        mat[col][row] = (T)ammonite::utils::randomDouble(1000000.0f);
      }
    }
  }

  template <typename T, unsigned int cols, unsigned int rows>
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
  template <typename T, unsigned int cols, unsigned int rows>
            requires ammonite::validMatrix<T, cols, rows>
  bool testEqual() {
    ammonite::Mat<T, cols, rows> aMat = {{0}};
    ammonite::Mat<T, cols, rows> bMat = {{0}};
    randomFillMatrix(aMat);

    //Set bMat to aMat
    for (unsigned int col = 0; col < cols; col++) {
      for (unsigned int row = 0; row < rows; row++) {
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

    for (unsigned int col = 0; col < cols; col++) {
      for (unsigned int row = 0; row < rows; row++) {
        //Safely guarantee a modification to bMat
        uintmax_t tmp = 0;
        std::memcpy(&tmp, &bMat[col][row], sizeof(bMat[col][row]));
        tmp ^= 1;
        std::memcpy(&bMat[col][row], &tmp, sizeof(bMat[col][row]));

        //Check ammonite::equal() on unequal matrices
        if (ammonite::equal(aMat, bMat)) {
          ammonite::utils::error << "Unequal matrix comparison failed" << std::endl;
          ammonite::utils::normal << "  Input:\n" << ammonite::formatMatrix(aMat) \
                                  << "\n  Input:\n" << ammonite::formatMatrix(bMat) \
                                  << std::endl;
          return false;
        }

        //Revert the change
        bMat[col][row] = aMat[col][row];
      }
    }

    return true;
  }

  template <typename T, unsigned int cols, unsigned int rows>
            requires ammonite::validMatrix<T, cols, rows>
  bool testCopy() {
    ammonite::Mat<T, cols, rows> aMat = {{0}};
    ammonite::Mat<T, cols, rows> bMat = {{0}};
    randomFillMatrix(aMat);

    ammonite::copy(aMat, bMat);
    if (!ammonite::equal(aMat, bMat)) {
      ammonite::utils::error << "Matrix copy failed" << std::endl;
      ammonite::utils::normal << "  Result:\n" << ammonite::formatMatrix(bMat) \
                              << "\n  Expected:\n" << ammonite::formatMatrix(aMat) \
                              << std::endl;
      return false;
    }

    //Check matrices are fully preserved when copying to a max column count matrix
    ammonite::Mat<T, 4, rows> cMat = {{0}};
    ammonite::copy(aMat, bMat);
    ammonite::copy(aMat, cMat);
    ammonite::copy(cMat, aMat);
    if (!ammonite::equal(aMat, bMat)) {
      ammonite::utils::error << "Matrix column count grow copy failed" << std::endl;
      ammonite::utils::normal << "  Result:\n" << ammonite::formatMatrix(aMat) \
                              << "\n  Expected:\n" << ammonite::formatMatrix(bMat) \
                              << std::endl;
      return false;
    }

    //Check matrices are fully preserved when copying to a min column count matrix
    ammonite::Mat<T, 2, rows> dMat = {{0}};
    ammonite::copy(aMat, dMat);
    for (unsigned int col = 0; col < 2; col++) {
      for (unsigned int row = 0; row < rows; row++) {
        if (aMat[col][row] != dMat[col][row]) {
          ammonite::utils::error << "Matrix column count shrink copy failed" << std::endl;
          ammonite::utils::normal << "  Result:\n" << ammonite::formatMatrix(dMat) \
                                  << "\n  Expected:\n" << ammonite::formatMatrix(aMat) \
                                  << std::endl;
          return false;
        }
      }
    }

    //Check matrices are fully preserved when copying to a max row count matrix
    ammonite::Mat<T, cols, 4> eMat = {{0}};
    ammonite::copy(aMat, bMat);
    ammonite::copy(aMat, eMat);
    ammonite::copy(eMat, aMat);
    if (!ammonite::equal(aMat, bMat)) {
      ammonite::utils::error << "Matrix row count grow copy failed" << std::endl;
      ammonite::utils::normal << "  Result:\n" << ammonite::formatMatrix(aMat) \
                              << "\n  Expected:\n" << ammonite::formatMatrix(bMat) \
                              << std::endl;
      return false;
    }

    //Check matrices are fully preserved when copying to a min row count matrix
    ammonite::Mat<T, cols, 2> fMat = {{0}};
    ammonite::copy(aMat, fMat);
    for (unsigned int col = 0; col < cols; col++) {
      for (unsigned int row = 0; row < 2; row++) {
        if (aMat[col][row] != fMat[col][row]) {
          ammonite::utils::error << "Matrix row count shrink copy failed" << std::endl;
          ammonite::utils::normal << "  Result:\n" << ammonite::formatMatrix(fMat) \
                                  << "\n  Expected:\n" << ammonite::formatMatrix(aMat) \
                                  << std::endl;
          return false;
        }
      }
    }

    return true;
  }

  template <typename T, unsigned int cols, unsigned int rows>
            requires ammonite::validMatrix<T, cols, rows>
  bool testCopyCast() {
    ammonite::Mat<T, cols, rows> aMat = {{0}};
    ammonite::Mat<double, cols, rows> bMat = {{0}};
    randomFillMatrix(aMat);

    ammonite::copyCast(aMat, bMat);
    for (unsigned int col = 0; col < cols; col++) {
      for (unsigned int row = 0; row < rows; row++) {
        if ((double)aMat[col][row] != bMat[col][row]) {
          ammonite::utils::error << "Matrix copy cast failed" << std::endl;
          ammonite::utils::normal << "  Result:\n" << ammonite::formatMatrix(bMat) \
                                  << "\n  Expected:\n" << ammonite::formatMatrix(aMat) \
                                  << std::endl;
          return false;
        }
      }
    }

    //Check matrices are fully preserved when copying to a max column count matrix
    ammonite::Mat<double, 4, rows> cMat = {{0}};
    ammonite::copyCast(aMat, cMat);
    for (unsigned int col = 0; col < cols; col++) {
      for (unsigned int row = 0; row < rows; row++) {
        if ((double)aMat[col][row] != cMat[col][row]) {
          ammonite::utils::error << "Matrix column count grow copy cast failed" << std::endl;
          ammonite::utils::normal << "  Result:\n" << ammonite::formatMatrix(cMat) \
                                  << "\n  Expected:\n" << ammonite::formatMatrix(aMat) \
                                  << std::endl;
          return false;
        }
      }
    }

    //Check matrices are fully preserved when copying to a min column count matrix
    ammonite::Mat<double, 2, rows> dMat = {{0}};
    ammonite::copyCast(aMat, dMat);
    for (unsigned int col = 0; col < 2; col++) {
      for (unsigned int row = 0; row < rows; row++) {
        if ((double)aMat[col][row] != dMat[col][row]) {
          ammonite::utils::error << "Matrix column count shrink copy cast failed" << std::endl;
          ammonite::utils::normal << "  Result:\n" << ammonite::formatMatrix(dMat) \
                                  << "\n  Expected:\n" << ammonite::formatMatrix(aMat) \
                                  << std::endl;
          return false;
        }
      }
    }

    //Check matrices are fully preserved when copying to a max row count matrix
    ammonite::Mat<double, cols, 4> eMat = {{0}};
    ammonite::copyCast(aMat, eMat);
    for (unsigned int col = 0; col < cols; col++) {
      for (unsigned int row = 0; row < rows; row++) {
        if ((double)aMat[col][row] != eMat[col][row]) {
          ammonite::utils::error << "Matrix row count grow copy cast failed" << std::endl;
          ammonite::utils::normal << "  Result:\n" << ammonite::formatMatrix(eMat) \
                                  << "\n  Expected:\n" << ammonite::formatMatrix(aMat) \
                                  << std::endl;
          return false;
        }
      }
    }

    //Check matrices are fully preserved when copying to a min row count matrix
    ammonite::Mat<double, cols, 2> fMat = {{0}};
    ammonite::copyCast(aMat, fMat);
    for (unsigned int col = 0; col < cols; col++) {
      for (unsigned int row = 0; row < 2; row++) {
        if ((double)aMat[col][row] != fMat[col][row]) {
          ammonite::utils::error << "Matrix row count shrink copy cast failed" << std::endl;
          ammonite::utils::normal << "  Result:\n" << ammonite::formatMatrix(fMat) \
                                  << "\n  Expected:\n" << ammonite::formatMatrix(aMat) \
                                  << std::endl;
          return false;
        }
      }
    }

    return true;
  }

  template <typename T, unsigned int cols, unsigned int rows>
            requires ammonite::validMatrix<T, cols, rows>
  bool testDiagonal() {
    constexpr unsigned int minSize = std::min(cols, rows);
    ammonite::Mat<T, cols, rows> aMat = {{0}};
    ammonite::Vec<T, minSize> minLengthVec = {0};
    randomFillVector(minLengthVec);

    //Test scalar diagonal
    ammonite::diagonal(aMat, minLengthVec[0]);
    for (unsigned int i = 0; i < minSize; i++) {
      if (aMat[i][i] != minLengthVec[0]) {
        ammonite::utils::error << "Matrix scalar diagonal set failed" << std::endl;
        ammonite::utils::normal << "  Result:\n" << ammonite::formatMatrix(aMat) \
                                << "\n  Expected: " << minLengthVec[0] \
                                << " at column " << i << ", row " << i << std::endl;
        return false;
      }
    }

    //Test vector diagonal
    ammonite::diagonal(aMat, minLengthVec);
    for (unsigned int i = 0; i < minSize; i++) {
      if (aMat[i][i] != minLengthVec[i]) {
        ammonite::utils::error << "Matrix vector diagonal set failed" << std::endl;
        ammonite::utils::normal << "  Result:\n" << ammonite::formatMatrix(aMat) \
                                << "\n  Expected: " << minLengthVec[i] \
                                << " at column " << i << ", row " << i << std::endl;
        return false;
      }
    }

    return true;
  }
}

namespace tests {
  template <typename T, unsigned int cols, unsigned int rows>
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

      //Test ammonite::diagonal()
      if (!testDiagonal<T, cols, rows>()) {
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
