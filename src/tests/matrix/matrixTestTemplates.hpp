#ifndef MATRIXTESTTEMPLATES
#define MATRIXTESTTEMPLATES

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string_view>

#include <ammonite/ammonite.hpp>

#include "../support.hpp"

namespace {
  template <typename T, unsigned int cols, unsigned int rows>
            requires ammonite::validMatrix<T, cols, rows>
  bool testData() {
    ammonite::Mat<T, cols, rows> aMat = {{0}};

    if ((void*)ammonite::data(aMat) != (void*)&aMat) {
      ammonite::utils::error << "Data pointer has a different address to the matrix" << std::endl;
      ammonite::utils::normal << "  Result:   " << (void*)ammonite::data(aMat) \
                              << "\n  Expected: " << (void*)&aMat << std::endl;
      return false;
    }

    return true;
  }

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
  bool testSet() {
    ammonite::Mat<T, cols, rows> aMat = {{0}};
    T a = randomScalar<T>();
    randomFillMatrix(aMat);

    //Test scalar set
    ammonite::set(aMat, a);
    for (unsigned int col = 0; col < cols; col++) {
      for (unsigned int row = 0; row < rows; row++) {
        if (aMat[col][row] != a) {
          ammonite::utils::error << "Matrix scalar set failed" << std::endl;
          ammonite::utils::normal << "  Result:\n" << ammonite::formatMatrix(aMat) \
                                  << "\n  Expected: " << a \
                                  << " at column " << col << ", row " << row << std::endl;
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

    //Test matrix identity
    ammonite::identity(aMat);
    for (unsigned int i = 0; i < minSize; i++) {
      if (aMat[i][i] != (T)1.0) {
        ammonite::utils::error << "Matrix identity set failed" << std::endl;
        ammonite::utils::normal << "  Result:\n" << ammonite::formatMatrix(aMat) \
                                << "\n  Expected: " << (T)1.0 \
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
  bool testMultiply() {
    ammonite::Mat<T, colsA, rowsA> aMat = {{0}};
    ammonite::Mat<T, colsB, colsA> bMat = {{0}};
    ammonite::Mat<T, colsB, rowsA> cMat = {{0}};
    ammonite::Vec<T, colsA> aVec = {{0}};
    ammonite::Vec<T, rowsA> bVec = {{0}};
    randomFillMatrix(aMat);
    randomFillMatrix(bMat);
    randomFillVector(aVec);

    //Test matrix-matrix multiplication
    ammonite::multiply(aMat, bMat, cMat);
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
      ammonite::multiply(aMat, bMat);
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
    ammonite::multiply(aMat, aVec, bVec);
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
      ammonite::multiply(aMat, aVec);
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
    ammonite::multiply(aMat, aVec[0], eMat);
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
    ammonite::multiply(eMat, aVec[0]);
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

  template <typename T, unsigned int cols, unsigned int rows>
            requires ammonite::validMatrix<T, cols, rows>
  bool testDeterminant() {
    if constexpr (cols == rows) {
      ammonite::Mat<T, cols, rows> aMat = {{0}};
      ammonite::Vec<T, cols> aVec = {0};
      randomFillVector(aVec);

      T expected = (T)1.0;
      for (unsigned int i = 0; i < cols; i++) {
        expected *= aVec[i];
      }

      //Test determinant
      ammonite::diagonal(aMat, aVec);
      T determinant = ammonite::determinant(aMat);
      if (!roughly(determinant, expected)) {
        ammonite::utils::error << "Matrix determinant failed" << std::endl;
        ammonite::utils::normal << "  Input:\n" << ammonite::formatMatrix(aMat) \
                                << "\n  Result: " << determinant \
                                << "\n  Expected: " << expected \
                                << std::endl;
        return false;
      }
    }

    return true;
  }

  template <typename T, unsigned int cols, unsigned int rows>
            requires ammonite::validMatrix<T, cols, rows>
  bool testInverse() {
    //Only run tests for square matrices with enough precision for intermediates
    if constexpr ((cols == rows) && sizeof(T) >= 8) {
      ammonite::Mat<T, cols, rows> aMat = {{0}};
      ammonite::Mat<T, cols, rows> bMat = {{0}};
      ammonite::Mat<T, cols, rows> cMat = {{0}};
      randomFillMatrix(aMat, (T)10);

      //Test matrix inverse if aMat is invertible
      if (ammonite::determinant(aMat) != (T)0) {
        ammonite::Mat<T, cols, rows> identityMat = {{0}};
        ammonite::identity(identityMat);

        ammonite::inverse(aMat, bMat);
        ammonite::multiply(aMat, bMat, cMat);
        for (unsigned int col = 0; col < cols; col++) {
          for (unsigned int row = 0; row < rows; row++) {
            if (!roughly(std::round(cMat[col][row]), identityMat[col][row], (T)0.001)) {
              ammonite::utils::error << "Matrix inverse failed" << std::endl;
              ammonite::utils::normal << "  Input:\n" << ammonite::formatMatrix(aMat) \
                                      << "\n  Result:\n" << ammonite::formatMatrix(bMat) \
                                      << "\n  Product:\n" << ammonite::formatMatrix(cMat) \
                                      << "\n  Expected: " << identityMat[col][row] \
                                      << " at product column " << col << ", row " << row \
                                      << std::endl;
              return false;
            }
          }
        }
      }
    }

    return true;
  }

  template <typename T, unsigned int cols, unsigned int rows>
            requires ammonite::validMatrix<T, cols, rows>
  bool testRotate() {
    //Test rotation matrix calculation
    if constexpr (cols == 4 && rows == 4) {
      const ammonite::Vec<T, 4> x = {(T)1.0, (T)0.0, (T)0.0, (T)0.0};
      const ammonite::Vec<T, 4> y = {(T)0.0, (T)1.0, (T)0.0, (T)0.0};
      const ammonite::Vec<T, 4> z = {(T)0.0, (T)0.0, (T)1.0, (T)0.0};
      const ammonite::Vec<T, 4> negY = {(T)0.0, (T)-1.0, (T)0.0, (T)0.0};

      //Calculate 3D normalised vector between x and z
      ammonite::Vec<T, 4> xz = {0};
      ammonite::add(x, z, xz);
      ammonite::normalise(xz);

      //Calculate 3D normalised vector between x, y and z
      ammonite::Vec<T, 4> xyz = {0};
      ammonite::add(x, y, xyz);
      ammonite::add(xyz, z, xyz);
      ammonite::normalise(xyz);

      //NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
      struct TestData {
        const ammonite::Vec<T, 4>& axis;
        const ammonite::Vec<T, 4>& in;
        const ammonite::Vec<T, 4>& out;
        T angle;
      };
      //NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)

      TestData tests[11] = {
        //Convert between axes
        {y, x, z, -ammonite::pi<T>() / 2},
        {z, y, x, -ammonite::pi<T>() / 2},
        {x, z, y, -ammonite::pi<T>() / 2},

        //Reflect axes
        {xz, x, z, ammonite::pi<T>()},
        {xz, z, x, ammonite::pi<T>()},
        {xz, y, negY, ammonite::pi<T>()},

        //Roll axes
        {xyz, y, z, (ammonite::pi<T>() * 2) / 3},
        {xyz, z, x, (ammonite::pi<T>() * 2) / 3},
        {xyz, x, y, (ammonite::pi<T>() * 2) / 3},

        //Recover x and z from xz
        {y, xz, x, ammonite::pi<T>() / 4},
        {y, xz, z, -ammonite::pi<T>() / 4}
      };

      const int totalTests = sizeof(tests) / sizeof(TestData);
      for (int testIndex = 0; testIndex < totalTests; testIndex++) {
        //Prepare matrix storage
        ammonite::Mat<T, 4> identityMat = {{0}};
        ammonite::Mat<T, 4> rotMat = {{0}};
        ammonite::identity(identityMat);

        //Correct the axis vector size
        ammonite::Vec<T, 3> axis = {0};
        ammonite::copy(tests[testIndex].axis, axis);

        //Calculate the matrix and rotate the point
        ammonite::Vec<T, 4> result = {0};
        ammonite::rotate(identityMat, tests[testIndex].angle, axis, rotMat);
        ammonite::multiply(rotMat, tests[testIndex].in, result);

        //Check calculated point matches expected
        for (unsigned int i = 0; i < 4; i++) {
          if (!roughly(result[i], tests[testIndex].out[i])) {
            ammonite::utils::error << "Matrix rotate failed" << std::endl;
            ammonite::utils::normal << "  Input axis:\n" << ammonite::formatVector(axis) \
                                    << "\n  Input point:\n" << ammonite::formatVector(tests[testIndex].in) \
                                    << "\n  Rotation matrix:\n" << ammonite::formatMatrix(rotMat) \
                                    << "\n  Output point:\n" << ammonite::formatVector(result) \
                                    << "\n  Expected output point:\n" << ammonite::formatVector(tests[testIndex].out) \
                                    << std::endl;
            return false;
          }
        }

        //Create the rotation matrix in-place, then verify it
        ammonite::Mat<T, 4> newRotMat = {{0}};
        ammonite::identity(newRotMat);
        ammonite::rotate(newRotMat, tests[testIndex].angle, axis);
        if (!ammonite::equal(newRotMat, rotMat)) {
          ammonite::utils::error << "In-place matrix rotate failed" << std::endl;
          ammonite::utils::normal << "  Result:\n" << ammonite::formatMatrix(newRotMat) \
                                  << "\n  Expected:\n" << ammonite::formatMatrix(rotMat) \
                                  << std::endl;
          return false;
        }
      }
    }

    return true;
  }

  template <typename T, unsigned int cols, unsigned int rows>
            requires ammonite::validMatrix<T, cols, rows>
  bool testScale() {
    //Test scale matrix calculation
    if constexpr (cols == 4 && rows == 4) {
      //Prepare input and ratios
      ammonite::Vec<T, 4> inVec = {0};
      ammonite::Vec<T, 4> outVec = {0};
      ammonite::Vec<T, 3> scaleVec = {0};
      randomFillVector(inVec);
      randomFillVector(scaleVec);
      inVec[3] = (T)1.0;

      //Create the scale matrix
      ammonite::Mat<T, 4> identityMat = {{0}};
      ammonite::Mat<T, 4> scaleMat = {{0}};
      ammonite::identity(identityMat);
      ammonite::scale(identityMat, scaleVec, scaleMat);

      //Scale the point and verify it
      ammonite::multiply(scaleMat, inVec, outVec);
      for (unsigned int i = 0; i < 3; i++) {
        if (!roughly(inVec[i] * scaleVec[i], outVec[i])) {
          ammonite::utils::error << "Matrix scale failed" << std::endl;
          ammonite::utils::normal << "  Input scale:\n" << ammonite::formatVector(scaleVec) \
                                  << "\n  Input point:\n" << ammonite::formatVector(inVec) \
                                  << "\n  Scale matrix:\n" << ammonite::formatMatrix(scaleMat) \
                                  << "\n  Output point:\n" << ammonite::formatVector(outVec) \
                                  << "\n  Expected: " << inVec[i] * scaleVec[i] \
                                  << " at index " << i << std::endl;
          return false;
        }
      }

      //Create the scale matrix in-place, then verify it
      ammonite::Mat<T, 4> newScaleMat = {{0}};
      ammonite::identity(newScaleMat);
      ammonite::scale(newScaleMat, scaleVec);
      if (!ammonite::equal(newScaleMat, scaleMat)) {
        ammonite::utils::error << "In-place matrix scale failed" << std::endl;
        ammonite::utils::normal << "  Result:\n" << ammonite::formatMatrix(newScaleMat) \
                                << "\n  Expected:\n" << ammonite::formatMatrix(scaleMat) \
                                << std::endl;
        return false;
      }
    }

    return true;
  }

  template <typename T, unsigned int cols, unsigned int rows>
            requires ammonite::validMatrix<T, cols, rows>
  bool testTranslate() {
    //Test translation matrix calculation
    if constexpr (cols == 4 && rows == 4) {
      //Prepare input and translation
      ammonite::Vec<T, 4> inVec = {0};
      ammonite::Vec<T, 4> outVec = {0};
      ammonite::Vec<T, 3> translationVec = {0};
      randomFillVector(inVec);
      randomFillVector(translationVec);
      inVec[3] = (T)1.0;

      //Create the translation matrix
      ammonite::Mat<T, 4> identityMat = {{0}};
      ammonite::Mat<T, 4> translationMat = {{0}};
      ammonite::identity(identityMat);
      ammonite::translate(identityMat, translationVec, translationMat);

      //Translate the point and verify it
      ammonite::multiply(translationMat, inVec, outVec);
      for (unsigned int i = 0; i < 3; i++) {
        if (!roughly(inVec[i] + translationVec[i], outVec[i])) {
          ammonite::utils::error << "Matrix translation failed" << std::endl;
          ammonite::utils::normal << "  Input translation:\n" << ammonite::formatVector(translationVec) \
                                  << "\n  Input point:\n" << ammonite::formatVector(inVec) \
                                  << "\n  Translation matrix:\n" << ammonite::formatMatrix(translationMat) \
                                  << "\n  Output point:\n" << ammonite::formatVector(outVec) \
                                  << "\n  Expected: " << inVec[i] + translationVec[i] \
                                  << " at index " << i << std::endl;
          return false;
        }
      }

      //Create the translation matrix in-place, then verify it
      ammonite::Mat<T, 4> newTranslationMat = {{0}};
      ammonite::identity(newTranslationMat);
      ammonite::translate(newTranslationMat, translationVec);
      if (!ammonite::equal(newTranslationMat, translationMat)) {
        ammonite::utils::error << "In-place matrix translation failed" << std::endl;
        ammonite::utils::normal << "  Result:\n" << ammonite::formatMatrix(newTranslationMat) \
                                << "\n  Expected:\n" << ammonite::formatMatrix(translationMat) \
                                << std::endl;
        return false;
      }
    }

    return true;
  }

  template <typename T, unsigned int cols, unsigned int rows>
            requires ammonite::validMatrix<T, cols, rows>
  bool testLookAt() {
    //Test view matrix calculation
    if constexpr ((cols == 4 && rows == 4) && sizeof(T) >= 8) {
      //Prepare view matrix parameters
      ammonite::Vec<T, 3> cameraVec = {0};
      ammonite::Vec<T, 3> targetVec = {0};
      ammonite::Vec<T, 3> upVec = {0};
      randomFillVector(cameraVec, (T)10.0);
      randomFillVector(targetVec, (T)10.0);
      randomFillVector(upVec, (T)10.0);
      ammonite::normalise(upVec);

      //Filter out scenarios where the camera is exactly up or down
      ammonite::Vec<T, 3> cameraDirectionVec = {0};
      ammonite::sub(targetVec, cameraVec, cameraDirectionVec);
      ammonite::normalise(cameraDirectionVec);
      for (unsigned int i = 0; i < 3; i++) {
        if (roughly(cameraDirectionVec[i], upVec[i])) {
          return true;
        }
      }

      //Create the view matrix
      ammonite::Mat<T, 4> viewMat = {{0}};
      ammonite::lookAt(cameraVec, targetVec, upVec, viewMat);

      //Vectors for tests
      ammonite::Vec<T, 4> originWideVec = {0};
      ammonite::Vec<T, 4> cameraWideVec = {0};
      ammonite::Vec<T, 4> targetWideVec = {0};
      ammonite::Vec<T, 4> negZVec = {(T)0.0, (T)0.0, (T)-1.0, (T)0.0};
      ammonite::copy(cameraVec, cameraWideVec);
      ammonite::copy(targetVec, targetWideVec);

      //Coordinate 1 unit away from the camera, towards the target
      ammonite::Vec<T, 4> cameraTargetPosWideVec = {0};
      ammonite::sub(targetWideVec, cameraWideVec, cameraTargetPosWideVec);
      ammonite::normalise(cameraTargetPosWideVec);
      ammonite::add(cameraTargetPosWideVec, cameraWideVec, cameraTargetPosWideVec);

      //Set 4th components
      cameraWideVec[3] = (T)1.0;
      originWideVec[3] = (T)1.0;
      cameraTargetPosWideVec[3] = (T)1.0;
      negZVec[3] = (T)1.0;

      //NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
      struct TestData {
        const ammonite::Vec<T, 4>& in;
        const ammonite::Vec<T, 4>& out;
      };
      //NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)

      TestData tests[2] = {
        {cameraWideVec, originWideVec},
        {cameraTargetPosWideVec, negZVec}
      };

      const int totalTests = sizeof(tests) / sizeof(TestData);
      for (int testIndex = 0; testIndex < totalTests; testIndex++) {
        //Apply the view matrix and verify it
        ammonite::Vec<T, 4> outVec = {0};
        ammonite::multiply(viewMat, tests[testIndex].in, outVec);

        for (unsigned int i = 0; i < 3; i++) {
          if (!roughly(tests[testIndex].out[i], outVec[i])) {
            ammonite::utils::error << "View matrix calculation failed" << std::endl;
            ammonite::utils::normal << "  Input camera position:\n" << ammonite::formatVector(cameraVec) \
                                    << "\n  Input target position:\n" << ammonite::formatVector(targetVec) \
                                    << "\n  Input up vector:\n" << ammonite::formatVector(upVec) \
                                    << "\n  Input point:\n" << ammonite::formatVector(tests[testIndex].in) \
                                    << "\n  View matrix:\n" << ammonite::formatMatrix(viewMat) \
                                    << "\n  Output point:\n" << ammonite::formatVector(outVec) \
                                    << "\n  Expected:\n" << ammonite::formatVector(tests[testIndex].out) \
                                    << std::endl;
            return false;
          }
        }
      }
    }

    return true;
  }

  template <typename T, unsigned int cols, unsigned int rows>
            requires ammonite::validMatrix<T, cols, rows>
  bool testPerspective() {
    //Test perspective projection matrix calculation
    if constexpr ((cols == 4 && rows == 4)) {
      ammonite::Vec<T, 4> inVec = {(T)0.0, (T)0.0, (T)0.0, (T)1.0};
      ammonite::Vec<T, 4> outVec = {0};

      //Pick random matrix parameters and calculate the matrix
      T fov = randomScalar<T>();
      T aspectRatio = randomScalar<T>();
      T nearPlane = (T)0.1;
      T farPlane = (T)100.0;
      ammonite::Mat<T, 4> perspectiveMat = {{0}};
      ammonite::perspective(fov, aspectRatio, nearPlane, farPlane, perspectiveMat);

      //Test near plane perspective divide
      inVec[2] = -nearPlane;
      ammonite::multiply(perspectiveMat, inVec, outVec);
      ammonite::divide(outVec, outVec[3]);
      if (!roughly(outVec[2], -(T)1.0)) {
        ammonite::utils::error << "Perspective projection matrix calculation failed" << std::endl;
        ammonite::utils::normal << "  Input field of view: " << fov \
                                << "\n  Input aspect ratio: " << aspectRatio \
                                << "\n  Input near plane: " << nearPlane \
                                << "\n  Input far plane: " << farPlane \
                                << "\n  Perspective projection matrix:\n" << ammonite::formatMatrix(perspectiveMat) \
                                << "\n  Output vector:\n" << ammonite::formatVector(outVec) \
                                << "\n  Expected: " << -(T)1.0 << " at index 2 " \
                                << std::endl;
        return false;
      }

      //Test far plane perspective divide
      inVec[2] = -farPlane;
      ammonite::multiply(perspectiveMat, inVec, outVec);
      ammonite::divide(outVec, outVec[3]);
      if (!roughly(outVec[2], (T)1.0)) {
        ammonite::utils::error << "Perspective projection matrix calculation failed" << std::endl;
        ammonite::utils::normal << "  Input field of view: " << fov \
                                << "\n  Input aspect ratio: " << aspectRatio \
                                << "\n  Input near plane: " << nearPlane \
                                << "\n  Input far plane: " << farPlane \
                                << "\n  Perspective projection matrix:\n" << ammonite::formatMatrix(perspectiveMat) \
                                << "\n  Output vector:\n" << ammonite::formatVector(outVec) \
                                << "\n  Expected: " << (T)1.0 << " at index 2 " \
                                << std::endl;
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

    //Test ammonite::data()
    if (!testData<T, cols, rows>()) {
      return false;
    }

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

      //Test ammonite::set()
      if (!testSet<T, cols, rows>()) {
        return false;
      }

      //Test ammonite::diagonal() and ammonite::identity()
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

      //Test ammonite::multiply()
      if (!testMultiply<T, cols, rows, 2>()) {
        return false;
      }
      if (!testMultiply<T, cols, rows, 3>()) {
        return false;
      }
      if (!testMultiply<T, cols, rows, 4>()) {
        return false;
      }

      //Test ammonite::determinant()
      if (!testDeterminant<T, cols, rows>()) {
        return false;
      }

      //Test ammonite::inverse()
      if (!testInverse<T, cols, rows>()) {
        return false;
      }

      //Test ammonite::scale()
      if (!testScale<T, cols, rows>()) {
        return false;
      }

      //Test ammonite::translate()
      if (!testTranslate<T, cols, rows>()) {
        return false;
      }

      //Test ammonite::lookAt()
      if (!testLookAt<T, cols, rows>()) {
        return false;
      }

      //Test ammonite::perspective()
      if (!testPerspective<T, cols, rows>()) {
        return false;
      }
    }

    //Test ammonite::rotate()
    return testRotate<T, cols, rows>();
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
