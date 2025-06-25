#ifndef AMMONITEMATRIX
#define AMMONITEMATRIX

#include <algorithm>
#include <cstring>
#include <string>
#include <type_traits>

//TODO: Convert to <simd> with C++26, drop header filter regex
//NOLINTNEXTLINE(misc-include-cleaner)
#include <experimental/simd>

#include "matrixTypes.hpp"
#include "vector.hpp"

namespace ammonite {
  //Maths operations
  inline namespace maths {
    //Copy from src to dest for two equally sized and typed matrices
    template <typename T, std::size_t cols, std::size_t rows> requires validMatrix<T, cols, rows>
    constexpr void copy(const Mat<T, cols, rows>& src, Mat<T, cols, rows>& dest) {
      if consteval {
        //Slower, constexpr-friendly copy
        std::copy(&src[0][0], &src[cols - 1][rows], &dest[0][0]);
      } else {
        std::memcpy(&dest[0], &src[0], sizeof(Mat<T, cols, rows>));
      }
    }

    //Copy from src to dest for two equally typed but differently sized matrices
    template <typename T, std::size_t colsA, std::size_t rowsA, std::size_t colsB, std::size_t rowsB>
              requires validMatrix<T, colsA, rowsA> && validMatrix<T, colsB, rowsB> &&
              (colsA != colsB || rowsA != rowsB)
    constexpr void copy(const Mat<T, colsA, rowsA>& src, Mat<T, colsB, rowsB>& dest) {
      constexpr std::size_t minCols = std::min(colsA, colsB);
      if constexpr (rowsA == rowsB) {
        //Columns are equally sized, copy up to the size of the smaller matrix
        if consteval {
          //Slower, constexpr-friendly copy
          std::copy(&src[0][0], &src[minCols - 1][rowsA], &dest[0][0]);
        } else {
          std::memcpy(&dest[0], &src[0], sizeof(Mat<T, minCols, rowsA>));
        }
      } else {
        //Columns are differently sized, copy each column individually
        for (std::size_t i = 0; i < minCols; i++) {
          ammonite::copy(src[i], dest[i]);
        }
      }
    }

    /*
     - Copy from src to dest for two equally sized but differently typed matrices
     - Cast every element during the copy
    */
    template <typename T, typename S, std::size_t cols, std::size_t rows>
              requires validMatrix<T, cols, rows> && validMatrix<S, cols, rows>
    constexpr void copyCast(const Mat<T, cols, rows>& src, Mat<S, cols, rows>& dest) {
      if constexpr (std::is_same_v<T, S>) {
        //Faster runtime copies for equal types
        ammonite::copy(src, dest);
      } else {
        std::copy(&src[0][0], &src[cols - 1][rows], &dest[0][0]);
      }
    }

    /*
     - Copy from src to dest for two differently typed and differently sized matrices
     - Cast every element during the copy
    */
    template <typename T, std::size_t colsA, std::size_t rowsA,
              typename S, std::size_t colsB, std::size_t rowsB>
              requires validMatrix<T, colsA, rowsA> && validMatrix<S, colsB, rowsB> &&
              (colsA != colsB || rowsA != rowsB)
    constexpr void copyCast(const Mat<T, colsA, rowsA>& src, Mat<S, colsB, rowsB>& dest) {
      if constexpr (std::is_same_v<T, S>) {
        //Faster runtime copies for equal types
        ammonite::copy(src, dest);
      } else {
        constexpr std::size_t minCols = std::min(colsA, colsB);
        if constexpr (rowsA == rowsB) {
          //Columns are equally sized, copy up to the size of the smaller matrix
          std::copy(&src[0][0], &src[minCols - 1][rowsA], &dest[0][0]);
        } else {
          //Columns are differently sized, copy each column individually
          for (std::size_t i = 0; i < minCols; i++) {
            ammonite::copyCast(src[i], dest[i]);
          }
        }
      }
    }

    //Return true if two matrices of the same size and type have identical elements
    template <typename T, std::size_t cols, std::size_t rows>
              requires validMatrix<T, cols, rows>
    constexpr bool equal(const Mat<T, cols, rows>& a, const Mat<T, cols, rows>& b) {
      //NOLINTBEGIN(readability-else-after-return)
      if consteval {
        //Slower, constexpr-friendly equality check
        for (std::size_t col = 0; col < cols; col++) {
          for (std::size_t row = 0; row < rows; row++) {
            if (a[col][row] != b[col][row]) {
              return false;
            }
          }
        }

        return true;
      } else {
        return (std::memcmp(&a[0], &b[0], sizeof(a)) == 0);
      }
      //NOLINTEND(readability-else-after-return)
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
