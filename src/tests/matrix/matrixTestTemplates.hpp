#ifndef MATRIXTESTTEMPLATES
#define MATRIXTESTTEMPLATES

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string_view>

#include <ammonite/ammonite.hpp>

#include "../support.hpp"

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

  template <typename T, unsigned int cols, unsigned int rows>
            requires ammonite::validMatrix<T, cols, rows>
  bool testAdd() {
    ammonite::Mat<T, cols, rows> aMat = {{0}};
    ammonite::Mat<T, cols, rows> bMat = {{0}};
    ammonite::Mat<T, cols, rows> cMat = {{0}};
    randomFillMatrix(aMat);
    randomFillMatrix(bMat);

    //Test regular addition
    ammonite::add(aMat, bMat, cMat);
    for (unsigned int col = 0; col < cols; col++) {
      for (unsigned int row = 0; row < rows; row++) {
        if ((T)(aMat[col][row] + bMat[col][row]) != cMat[col][row]) {
          ammonite::utils::error << "Matrix addition failed" << std::endl;
          ammonite::utils::normal << "  Input:\n" << ammonite::formatMatrix(aMat) \
                                  << "\n  Input:\n" << ammonite::formatMatrix(bMat) \
                                  << "\n  Result:\n" << ammonite::formatMatrix(cMat) \
                                  << std::endl;
          return false;
        }
      }
    }

    //Test in-place addition
    ammonite::copy(aMat, cMat);
    ammonite::add(cMat, bMat);
    for (unsigned int col = 0; col < cols; col++) {
      for (unsigned int row = 0; row < rows; row++) {
        if ((T)(aMat[col][row] + bMat[col][row]) != cMat[col][row]) {
          ammonite::utils::error << "In-place matrix addition failed" << std::endl;
          ammonite::utils::normal << "  Input:\n" << ammonite::formatMatrix(aMat) \
                                  << "\n  Input:\n" << ammonite::formatMatrix(bMat) \
                                  << "\n  Result:\n" << ammonite::formatMatrix(cMat) \
                                  << std::endl;
          return false;
        }
      }
    }

    //Test scalar addition
    ammonite::add(aMat, bMat[0][0], cMat);
    for (unsigned int col = 0; col < cols; col++) {
      for (unsigned int row = 0; row < rows; row++) {
        if ((T)(aMat[col][row] + bMat[0][0]) != cMat[col][row]) {
          ammonite::utils::error << "Scalar matrix addition failed" << std::endl;
          ammonite::utils::normal << "  Input:\n" << ammonite::formatMatrix(aMat) \
                                  << "\n  Input:\n" << bMat[0][0] \
                                  << "\n  Result:\n" << ammonite::formatMatrix(cMat) \
                                  << std::endl;
          return false;
        }
      }
    }

    //Test in-place scalar addition
    ammonite::copy(aMat, cMat);
    ammonite::add(cMat, bMat[0][0]);
    for (unsigned int col = 0; col < cols; col++) {
      for (unsigned int row = 0; row < rows; row++) {
        if ((T)(aMat[col][row] + bMat[0][0]) != cMat[col][row]) {
          ammonite::utils::error << "In-place scalar matrix addition failed" << std::endl;
          ammonite::utils::normal << "  Input:\n" << ammonite::formatMatrix(aMat) \
                                  << "\n  Input:\n" << bMat[0][0] \
                                  << "\n  Result:\n" << ammonite::formatMatrix(cMat) \
                                  << std::endl;
          return false;
        }
      }
    }

    return true;
  }

  template <typename T, unsigned int cols, unsigned int rows>
            requires ammonite::validMatrix<T, cols, rows>
  bool testSub() {
    ammonite::Mat<T, cols, rows> aMat = {{0}};
    ammonite::Mat<T, cols, rows> bMat = {{0}};
    ammonite::Mat<T, cols, rows> cMat = {{0}};
    randomFillMatrix(aMat);
    randomFillMatrix(bMat);

    //Test regular subtraction
    ammonite::sub(aMat, bMat, cMat);
    for (unsigned int col = 0; col < cols; col++) {
      for (unsigned int row = 0; row < rows; row++) {
        if ((T)(aMat[col][row] - bMat[col][row]) != cMat[col][row]) {
          ammonite::utils::error << "Matrix subtraction failed" << std::endl;
          ammonite::utils::normal << "  Input:\n" << ammonite::formatMatrix(aMat) \
                                  << "\n  Input:\n" << ammonite::formatMatrix(bMat) \
                                  << "\n  Result:\n" << ammonite::formatMatrix(cMat) \
                                  << std::endl;
          return false;
        }
      }
    }

    //Test in-place subtraction
    ammonite::copy(aMat, cMat);
    ammonite::sub(cMat, bMat);
    for (unsigned int col = 0; col < cols; col++) {
      for (unsigned int row = 0; row < rows; row++) {
        if ((T)(aMat[col][row] - bMat[col][row]) != cMat[col][row]) {
          ammonite::utils::error << "In-place matrix subtraction failed" << std::endl;
          ammonite::utils::normal << "  Input:\n" << ammonite::formatMatrix(aMat) \
                                  << "\n  Input:\n" << ammonite::formatMatrix(bMat) \
                                  << "\n  Result:\n" << ammonite::formatMatrix(cMat) \
                                  << std::endl;
          return false;
        }
      }
    }

    //Test scalar subtraction
    ammonite::sub(aMat, bMat[0][0], cMat);
    for (unsigned int col = 0; col < cols; col++) {
      for (unsigned int row = 0; row < rows; row++) {
        if ((T)(aMat[col][row] - bMat[0][0]) != cMat[col][row]) {
          ammonite::utils::error << "Scalar matrix subtraction failed" << std::endl;
          ammonite::utils::normal << "  Input:\n" << ammonite::formatMatrix(aMat) \
                                  << "\n  Input:\n" << bMat[0][0] \
                                  << "\n  Result:\n" << ammonite::formatMatrix(cMat) \
                                  << std::endl;
          return false;
        }
      }
    }

    //Test in-place scalar subtraction
    ammonite::copy(aMat, cMat);
    ammonite::sub(cMat, bMat[0][0]);
    for (unsigned int col = 0; col < cols; col++) {
      for (unsigned int row = 0; row < rows; row++) {
        if ((T)(aMat[col][row] - bMat[0][0]) != cMat[col][row]) {
          ammonite::utils::error << "In-place scalar matrix subtraction failed" << std::endl;
          ammonite::utils::normal << "  Input:\n" << ammonite::formatMatrix(aMat) \
                                  << "\n  Input:\n" << bMat[0][0] \
                                  << "\n  Result:\n" << ammonite::formatMatrix(cMat) \
                                  << std::endl;
          return false;
        }
      }
    }

    return true;
  }

  template <typename T, unsigned int cols, unsigned int rows>
            requires ammonite::validMatrix<T, cols, rows>
  bool testTranspose() {
    ammonite::Mat<T, cols, rows> aMat = {{0}};
    ammonite::Mat<T, rows, cols> bMat = {{0}};
    randomFillMatrix(aMat);

    //Test transpose
    ammonite::transpose(aMat, bMat);
    for (unsigned int col = 0; col < cols; col++) {
      for (unsigned int row = 0; row < rows; row++) {
        if (aMat[col][row] != bMat[row][col]) {
          ammonite::utils::error << "Matrix transpose failed" << std::endl;
          ammonite::utils::normal << "  Input:\n" << ammonite::formatMatrix(aMat) \
                                  << "\n  Result:\n" << ammonite::formatMatrix(bMat) \
                                  << "\n  Expected: " << aMat[col][row] \
                                  << " at output column " << row << ", row " << col \
                                  << std::endl;
          return false;
        }
      }
    }

    //Test in-place square matrix transpose
    if constexpr (cols == rows) {
      ammonite::copy(aMat, bMat);
      ammonite::transpose(aMat, aMat);
      for (unsigned int col = 0; col < cols; col++) {
        for (unsigned int row = 0; row < rows; row++) {
          if (aMat[col][row] != bMat[row][col]) {
            ammonite::utils::error << "In-place matrix transpose failed" << std::endl;
            ammonite::utils::normal << "  Input:\n" << ammonite::formatMatrix(bMat) \
                                    << "\n  Result:\n" << ammonite::formatMatrix(aMat) \
                                    << "\n  Expected: " << bMat[col][row] \
                                    << " at output column " << row << ", row " << col \
                                    << std::endl;
            return false;
          }
        }
      }
    }

    return true;
  }

  template <typename T, unsigned int colsA, unsigned int rowsA, unsigned int colsB>
            requires ammonite::validMatrix<T, colsA, rowsA> &&
            ammonite::validMatrix<T, colsB, colsA> && ammonite::validVector<T, colsA> &&
            ammonite::validVector<T, rowsA>
  bool testMul() {
    ammonite::Mat<T, colsA, rowsA> aMat = {{0}};
    ammonite::Mat<T, colsB, colsA> bMat = {{0}};
    ammonite::Mat<T, colsB, rowsA> cMat = {{0}};
    ammonite::Vec<T, colsA> aVec = {{0}};
    ammonite::Vec<T, rowsA> bVec = {{0}};
    randomFillMatrix(aMat);
    randomFillMatrix(bMat);
    randomFillVector(aVec);

    //Test matrix-matrix multiplication
    ammonite::mul(aMat, bMat, cMat);
    for (unsigned int col = 0; col < colsB; col++) {
      for (unsigned int row = 0; row < rowsA; row++) {
        //Calculate expected value for the current index
        T sum = (T)0.0;
        for (unsigned int i = 0; i < colsA; i++) {
          sum += aMat[i][row] * bMat[col][i];
        }

        //Check returned value matches
        if (!roughly(sum, cMat[col][row])) {
          ammonite::utils::error << "Matrix-matrix multiplication failed" << std::endl;
          ammonite::utils::normal << "  Input:\n" << ammonite::formatMatrix(aMat) \
                                  << "\n  Input:\n" << ammonite::formatMatrix(bMat) \
                                  << "\n  Result:\n" << ammonite::formatMatrix(cMat) \
                                  << "\n  Expected: " << sum \
                                  << " at column " << col << ", row " << row \
                                  << std::endl;
          return false;
        }
      }
    }

    //Test in-place matrix-matrix multiplication
    if constexpr ((colsA == rowsA) && (colsA == colsB)) {
      ammonite::Mat<T, colsA, rowsA> dMat = {{0}};
      ammonite::copy(aMat, dMat);
      ammonite::mul(aMat, bMat);
      for (unsigned int col = 0; col < colsA; col++) {
        for (unsigned int row = 0; row < rowsA; row++) {
          //Calculate expected value for the current index
          T sum = (T)0.0;
          for (unsigned int i = 0; i < colsA; i++) {
            sum += dMat[i][row] * bMat[col][i];
          }

          //Check returned value matches
          if (!roughly(sum, aMat[col][row])) {
            ammonite::utils::error << "In-place matrix-matrix multiplication failed" << std::endl;
            ammonite::utils::normal << "  Input:\n" << ammonite::formatMatrix(dMat) \
                                    << "\n  Input:\n" << ammonite::formatMatrix(bMat) \
                                    << "\n  Result:\n" << ammonite::formatMatrix(aMat) \
                                    << "\n  Expected: " << sum \
                                    << " at column " << col << ", row " << row \
                                    << std::endl;
            return false;
          }
        }
      }
    }

    //Test matrix-vector multiplication
    ammonite::mul(aMat, aVec, bVec);
    for (unsigned int row = 0; row < rowsA; row++) {
      //Calculate expected value for the current index
      T sum = (T)0.0;
      for (unsigned int col = 0; col < colsA; col++) {
        sum += aMat[col][row] * aVec[col];
      }

      //Check returned value matches
      if (!roughly(sum, bVec[row])) {
        ammonite::utils::error << "Matrix-vector multiplication failed" << std::endl;
        ammonite::utils::normal << "  Input:\n" << ammonite::formatMatrix(aMat) \
                                << "\n  Input:\n" << ammonite::formatVector(aVec) \
                                << "\n  Result:\n" << ammonite::formatVector(bVec) \
                                << "\n  Expected: " << sum \
                                << " at index " << row << std::endl;
        return false;
      }
    }

    //Test in-place matrix-vector multiplication
    if constexpr ((colsA == rowsA) && (colsA == colsB)) {
      ammonite::copy(aVec, bVec);
      ammonite::mul(aMat, aVec);
      for (unsigned int row = 0; row < rowsA; row++) {
        //Calculate expected value for the current index
        T sum = (T)0.0;
        for (unsigned int col = 0; col < colsA; col++) {
          sum += aMat[col][row] * bVec[col];
        }

        //Check returned value matches
        if (!roughly(sum, aVec[row])) {
          ammonite::utils::error << "In-place matrix-vector multiplication failed" << std::endl;
          ammonite::utils::normal << "  Input:\n" << ammonite::formatMatrix(aMat) \
                                  << "\n  Input:\n" << ammonite::formatVector(bVec) \
                                  << "\n  Result:\n" << ammonite::formatVector(aVec) \
                                  << "\n  Expected: " << sum \
                                  << " at index " << row << std::endl;
          return false;
        }
      }

      //Restore aVec
      ammonite::copy(bVec, aVec);
    }

    //Test scalar multiplication
    ammonite::Mat<T, colsA, rowsA> eMat = {{0}};
    ammonite::mul(aMat, aVec[0], eMat);
    for (unsigned int col = 0; col < colsA; col++) {
      for (unsigned int row = 0; row < rowsA; row++) {
        if ((T)(aMat[col][row] * aVec[0]) != eMat[col][row]) {
          ammonite::utils::error << "Matrix-scalar multiplication failed" << std::endl;
          ammonite::utils::normal << "  Input:\n" << ammonite::formatMatrix(aMat) \
                                  << "\n  Input:\n" << aVec[0] \
                                  << "\n  Result:\n" << ammonite::formatMatrix(eMat) \
                                  << "\n  Expected: " << (T)(aMat[col][row] * aVec[0]) \
                                  << " at output column " << col << ", row " << row \
                                  << std::endl;
          return false;
        }
      }
    }

    //Test in-place scalar multiplication
    ammonite::copy(aMat, eMat);
    ammonite::mul(eMat, aVec[0]);
    for (unsigned int col = 0; col < colsA; col++) {
      for (unsigned int row = 0; row < rowsA; row++) {
        if ((T)(aMat[col][row] * aVec[0]) != eMat[col][row]) {
          ammonite::utils::error << "In-place matrix-scalar multiplication failed" << std::endl;
          ammonite::utils::normal << "  Input:\n" << ammonite::formatMatrix(aMat) \
                                  << "\n  Input:\n" << aVec[0] \
                                  << "\n  Result:\n" << ammonite::formatMatrix(eMat) \
                                  << "\n  Expected: " << (T)(aMat[col][row] * aVec[0]) \
                                  << " at output column " << col << ", row " << row \
                                  << std::endl;
          return false;
        }
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

      //Test ammonite::add()
      if (!testAdd<T, cols, rows>()) {
        return false;
      }

      //Test ammonite::sub()
      if (!testSub<T, cols, rows>()) {
        return false;
      }

      //Test ammonite::transpose()
      if (!testTranspose<T, cols, rows>()) {
        return false;
      }

      //Test ammonite::mul()
      if (!testMul<T, cols, rows, 2>()) {
        return false;
      }
      if (!testMul<T, cols, rows, 3>()) {
        return false;
      }
      if (!testMul<T, cols, rows, 4>()) {
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
