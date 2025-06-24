#ifndef AMMONITEMATRIX
#define AMMONITEMATRIX

#include <algorithm>
#include <cstring>
#include <string>

//TODO: Convert to <simd> with C++26, drop header filter regex
//NOLINTNEXTLINE(misc-include-cleaner)
#include <experimental/simd>

#include "matrixTypes.hpp"

namespace ammonite {
  //Maths operations
  inline namespace maths {
    //Copy from src to dest for two equally sized and typed matrices
    template <typename T, std::size_t cols, std::size_t rows> requires validMatrix<T, cols, rows>
    void copy(const Mat<T, cols, rows>& src, Mat<T, cols, rows>& dest) {
      std::memcpy(&dest[0], &src[0], sizeof(Mat<T, cols, rows>));
    }

    //Copy from src to dest for two equally typed but differently sized matrices
    template <typename T, std::size_t colsA, std::size_t rowsA, std::size_t colsB, std::size_t rowsB>
              requires validMatrix<T, colsA, rowsA> && validMatrix<T, colsB, rowsB> &&
              (colsA != colsB || rowsA != rowsB)
    void copy(const Mat<T, colsA, rowsA>& src, Mat<T, colsB, rowsB>& dest) {
      constexpr std::size_t minCols = std::min(colsA, colsB);
      if constexpr (rowsA == rowsB) {
        //Columns are equally sized, copy up to the size of the smaller matrix
        std::memcpy(&dest[0], &src[0], sizeof(Mat<T, minCols, rowsA>));
      } else {
        //Columns are differently sized, copy each column individually
        for (std::size_t i = 0; i < minCols; i++) {
          copy(src[i], dest[i]);
        }
      }
    }

    /*
     - Copy from src to dest for two equally sized but differently typed matrices
     - Cast every element during the copy
    */
    template <typename T, typename S, std::size_t cols, std::size_t rows>
              requires validMatrix<T, cols, rows> && validMatrix<S, cols, rows>
    void copyCast(const Mat<T, cols, rows>& src, Mat<S, cols, rows>& dest) {
      std::copy(&src[0], &src[cols * rows], &dest[0]);
    }

    /*
     - Copy from src to dest for two differently typed and differently sized matrices
     - Cast every element during the copy
    */
    template <typename T, std::size_t colsA, std::size_t rowsA,
              typename S, std::size_t colsB, std::size_t rowsB>
              requires validMatrix<T, colsA, rowsA> && validMatrix<T, colsB, rowsB> &&
              (!std::is_same_v<T, S> && (colsA != colsB || rowsA != rowsB))
    void copyCast(const Mat<T, colsA, rowsA>& src, Mat<T, colsB, rowsB>& dest) {
      constexpr std::size_t minCols = std::min(colsA, colsB);
      if constexpr (rowsA == rowsB) {
        //Columns are equally sized, copy up to the size of the smaller matrix
        std::copy(&src[0], &src[minCols * rowsA], sizeof(Mat<T, minCols, rowsA>));
      } else {
        //Columns are differently sized, copy each column individually
        for (std::size_t i = 0; i < minCols; i++) {
          copy(src[i], dest[i]);
        }
      }
    }

    /*
     - Return true if two matrices of the same size and type have identical elements
    */
    template <typename T, std::size_t cols, std::size_t rows>
              requires validMatrix<T, cols, rows>
    bool equal(const Mat<T, cols, rows>& a, const Mat<T, cols, rows>& b) {
      return (std::memcmp(&a[0], &b[0], sizeof(a)) == 0);
    }
  }

  //Utility / support functions
  inline namespace maths {
    template<typename T, std::size_t cols, std::size_t rows>
             requires validMatrix<T, cols, rows>
    std::string formatMatrix(const Mat<T, cols, rows>& matrix) {
      std::string result;
      for (std::size_t row = 0; row < rows; row++) {
        if (row != 0) {
          result += "\n";
        }

        for (std::size_t col = 0; col < cols; col++) {
          if (col != 0) {
            result += ", ";
          }

          result += std::to_string(matrix[col][row]);
        }
      }

      return result;
    }
  }
}

#endif
