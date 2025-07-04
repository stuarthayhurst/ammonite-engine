#ifndef AMMONITEMATRIX
#define AMMONITEMATRIX

#include <algorithm>
#include <cstring>
#include <string>
#include <type_traits>

//TODO: Convert to <simd> with C++26, drop header filter regex
//NOLINTNEXTLINE(misc-include-cleaner)
#include <experimental/simd>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "matrixTypes.hpp"
#include "vector.hpp"
#include "vectorTypes.hpp"

namespace ammonite {
  //Maths operations
  inline namespace maths {
    //Copy from src to dest for two equally sized and typed matrices
    template <typename T, unsigned int cols, unsigned int rows>
              requires validMatrix<T, cols, rows>
    constexpr void copy(const Mat<T, cols, rows>& src, Mat<T, cols, rows>& dest) {
      if consteval {
        //Slower, constexpr-friendly copy
        std::copy(&src[0][0], &src[cols - 1][rows], &dest[0][0]);
      } else {
        std::memcpy(&dest[0], &src[0], sizeof(Mat<T, cols, rows>));
      }
    }

    //Copy from src to dest for two equally typed but differently sized matrices
    template <typename T, unsigned int colsA, unsigned int rowsA,
              unsigned int colsB, unsigned int rowsB>
              requires validMatrix<T, colsA, rowsA> && validMatrix<T, colsB, rowsB> &&
              (colsA != colsB || rowsA != rowsB)
    constexpr void copy(const Mat<T, colsA, rowsA>& src, Mat<T, colsB, rowsB>& dest) {
      constexpr unsigned int minCols = std::min(colsA, colsB);
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
        for (unsigned int i = 0; i < minCols; i++) {
          ammonite::copy(src[i], dest[i]);
        }
      }
    }

    /*
     - Copy from src to dest for two equally sized but differently typed matrices
     - Cast every element during the copy
    */
    template <typename T, typename S, unsigned int cols, unsigned int rows>
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
    template <typename T, unsigned int colsA, unsigned int rowsA,
              typename S, unsigned int colsB, unsigned int rowsB>
              requires validMatrix<T, colsA, rowsA> && validMatrix<S, colsB, rowsB> &&
              (colsA != colsB || rowsA != rowsB)
    constexpr void copyCast(const Mat<T, colsA, rowsA>& src, Mat<S, colsB, rowsB>& dest) {
      if constexpr (std::is_same_v<T, S>) {
        //Faster runtime copies for equal types
        ammonite::copy(src, dest);
      } else {
        constexpr unsigned int minCols = std::min(colsA, colsB);
        if constexpr (rowsA == rowsB) {
          //Columns are equally sized, copy up to the size of the smaller matrix
          std::copy(&src[0][0], &src[minCols - 1][rowsA], &dest[0][0]);
        } else {
          //Columns are differently sized, copy each column individually
          for (unsigned int i = 0; i < minCols; i++) {
            ammonite::copyCast(src[i], dest[i]);
          }
        }
      }
    }

    //Return true if two matrices of the same size and type have identical elements
    template <typename T, unsigned int cols, unsigned int rows>
              requires validMatrix<T, cols, rows>
    constexpr bool equal(const Mat<T, cols, rows>& a, const Mat<T, cols, rows>& b) {
      //NOLINTBEGIN(readability-else-after-return)
      if consteval {
        //Slower, constexpr-friendly equality check
        for (unsigned int col = 0; col < cols; col++) {
          for (unsigned int row = 0; row < rows; row++) {
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

    //Set the diagonal of the matrix to a scalar
    template <typename T, unsigned int cols, unsigned int rows>
              requires validMatrix<T, cols, rows>
    constexpr void diagonal(Mat<T, cols, rows>& a, T scalar) {
      constexpr unsigned int minSize = std::min(cols, rows);
      for (unsigned int i = 0; i < minSize; i++) {
        a[i][i] = scalar;
      }
    }

    /*
     - Set the diagonal of the matrix to a vector
     - The vector's length must match one dimension, and not exceed the other
    */
    template <typename T, unsigned int cols, unsigned int rows, unsigned int size>
              requires validMatrix<T, cols, rows> && validVector<T, size> &&
              (((size == cols) && (size <= rows)) ||
              ((size == rows) && (size <= cols)))
    constexpr void diagonal(Mat<T, cols, rows>& a, const Vec<T, size>& b) {
      for (unsigned int i = 0; i < size; i++) {
        a[i][i] = b[i];
      }
    }

    //Add two matrices of the same size and type, storing the result in dest
    template <typename T, unsigned int cols, unsigned int rows>
              requires validMatrix<T, cols, rows>
    void add(const Mat<T, cols, rows>& a, const Mat<T, cols, rows>& b, Mat<T, cols, rows>& dest) {
      std::experimental::fixed_size_simd<T, cols * rows> aSimd(&a[0][0], std::experimental::element_aligned);
      std::experimental::fixed_size_simd<T, cols * rows> bSimd(&b[0][0], std::experimental::element_aligned);

      aSimd += bSimd;
      aSimd.copy_to(&dest[0][0], std::experimental::element_aligned);
    }

    //Add two matrices of the same size and type, storing the result in the first matrix
    template <typename T, unsigned int cols, unsigned int rows>
              requires validMatrix<T, cols, rows>
    void add(Mat<T, cols, rows>& a, const Mat<T, cols, rows>& b) {
      add(a, b, a);
    }

    /*
     - Add a scalar to each element of a matrix, storing the result in dest
       - The scalar and elements must have the same type
    */
    template <typename T, unsigned int cols, unsigned int rows>
              requires validMatrix<T, cols, rows>
    void add(const Mat<T, cols, rows>& a, T b, Mat<T, cols, rows>& dest) {
      std::experimental::fixed_size_simd<T, cols * rows> aSimd(&a[0][0], std::experimental::element_aligned);
      std::experimental::fixed_size_simd<T, cols * rows> bSimd = b;

      aSimd += bSimd;
      aSimd.copy_to(&dest[0][0], std::experimental::element_aligned);
    }

    /*
     - Add a scalar to each element of a matrix, storing the result in the same matrix
       - The scalar and elements must have the same type
    */
    template <typename T, unsigned int cols, unsigned int rows>
              requires validMatrix<T, cols, rows>
    void add(Mat<T, cols, rows>& a, T b) {
      add(a, b, a);
    }

    //Subtract matrix b from matrix a of the same size and type, storing the result in dest
    template <typename T, unsigned int cols, unsigned int rows>
              requires validMatrix<T, cols, rows>
    void sub(const Mat<T, cols, rows>& a, const Mat<T, cols, rows>& b, Mat<T, cols, rows>& dest) {
      std::experimental::fixed_size_simd<T, cols * rows> aSimd(&a[0][0], std::experimental::element_aligned);
      std::experimental::fixed_size_simd<T, cols * rows> bSimd(&b[0][0], std::experimental::element_aligned);

      aSimd -= bSimd;
      aSimd.copy_to(&dest[0][0], std::experimental::element_aligned);
    }

    //Subtract matrix b from matrix a of the same size and type, storing the result in matrix a
    template <typename T, unsigned int cols, unsigned int rows>
              requires validMatrix<T, cols, rows>
    void sub(Mat<T, cols, rows>& a, const Mat<T, cols, rows>& b) {
      sub(a, b, a);
    }

    /*
     - Subtract a scalar from each element of a matrix, storing the result in dest
       - The scalar and elements must have the same type
    */
    template <typename T, unsigned int cols, unsigned int rows>
              requires validMatrix<T, cols, rows>
    void sub(const Mat<T, cols, rows>& a, T b, Mat<T, cols, rows>& dest) {
      std::experimental::fixed_size_simd<T, cols * rows> aSimd(&a[0][0], std::experimental::element_aligned);
      std::experimental::fixed_size_simd<T, cols * rows> bSimd = b;

      aSimd -= bSimd;
      aSimd.copy_to(&dest[0][0], std::experimental::element_aligned);
    }

    /*
     - Subtract a scalar from each element of a matrix, storing the result in the same matrix
       - The scalar and elements must have the same type
    */
    template <typename T, unsigned int cols, unsigned int rows>
              requires validMatrix<T, cols, rows>
    void sub(Mat<T, cols, rows>& a, T b) {
      sub(a, b, a);
    }

    //TODO: Implement with <simd>
    //Transpose a matrix
    template <typename T, unsigned int colsA, unsigned int rowsA>
              requires validMatrix<T, colsA, rowsA>
    void transpose(const Mat<T, colsA, rowsA>& src, Mat<T, rowsA, colsA>& dest) {
      glm::mat<colsA, rowsA, T, glm::defaultp> srcMat;
      glm::mat<rowsA, colsA, T, glm::defaultp> destMat;

      std::memcpy(glm::value_ptr(srcMat), &src[0], sizeof(Mat<T, colsA, rowsA>));

      destMat = glm::transpose(srcMat);
      std::memcpy(&dest[0], glm::value_ptr(destMat), sizeof(Mat<T, rowsA, colsA>));
    }

    //Transpose a square matrix in-place
    template <typename T, unsigned int size>
              requires validMatrix<T, size, size>
    void transpose(Mat<T, size, size>& src) {
      transpose(src, src);
    }

    //TODO: Implement with <simd>
    /*
     - Multiply a matrix by a matrix, storing the result in dest
       - The scalar and elements must have the same type
    */
    template <typename T, unsigned int colsA, unsigned int rowsA, unsigned int colsB>
              requires validMatrix<T, colsA, rowsA> && validMatrix<T, colsB, colsA> &&
              validMatrix<T, colsB, rowsA>
    void multiply(const Mat<T, colsA, rowsA>& a, const Mat<T, colsB, colsA>& b,
                  Mat<T, colsB, rowsA>& dest) {
      glm::mat<colsA, rowsA, T, glm::defaultp> aMat;
      glm::mat<colsB, colsA, T, glm::defaultp> bMat;
      glm::mat<colsB, rowsA, T, glm::defaultp> destMat;

      std::memcpy(glm::value_ptr(aMat), &a[0], sizeof(Mat<T, colsA, rowsA>));
      std::memcpy(glm::value_ptr(bMat), &b[0], sizeof(Mat<T, colsB, colsA>));

      destMat = aMat * bMat;
      std::memcpy(&dest[0], glm::value_ptr(destMat), sizeof(Mat<T, colsB, rowsA>));
    }

    /*
     - Multiply a matrix by a matrix, storing the result in the first matrix
       - Both matrices must be square with an equal size
       - The scalar and elements must have the same type
    */
    template <typename T, unsigned int size>
              requires validMatrix<T, size, size>
    void multiply(Mat<T, size, size>& a, const Mat<T, size, size>& b) {
      multiply(a, b, a);
    }

    //TODO: Implement with <simd>
    /*
     - Multiply a matrix by a vector, storing the result in dest
       - The scalar and elements must have the same type
    */
    template <typename T, unsigned int cols, unsigned int rows>
              requires validMatrix<T, cols, rows> && validVector<T, cols> &&
              validVector<T, rows>
    void multiply(const Mat<T, cols, rows>& a, const Vec<T, cols>& b, Vec<T, rows>& dest) {
      glm::mat<cols, rows, T, glm::defaultp> aMat;
      glm::vec<cols, T, glm::defaultp> bVec;
      glm::vec<rows, T, glm::defaultp> destVec;

      std::memcpy(glm::value_ptr(aMat), &a[0], sizeof(Mat<T, cols, rows>));
      std::memcpy(glm::value_ptr(bVec), &b[0], sizeof(Vec<T, cols>));

      destVec = aMat * bVec;
      std::memcpy(&dest[0], glm::value_ptr(destVec), sizeof(Vec<T, rows>));
    }

    /*
     - Multiply a square matrix by a vector, storing the result in the same vector
       - The scalar and elements must have the same type
    */
    template <typename T, unsigned int size>
              requires validMatrix<T, size, size> && validVector<T, size>
    void multiply(const Mat<T, size, size>& a, Vec<T, size>& b) {
      multiply(a, b, b);
    }

    /*
     - Multiply a matrix by a scalar, storing the result in dest
       - The scalar and elements must have the same type
    */
    template <typename T, unsigned int cols, unsigned int rows>
              requires validMatrix<T, cols, rows>
    void multiply(const Mat<T, cols, rows>& a, T b, Mat<T, cols, rows>& dest) {
      std::experimental::fixed_size_simd<T, cols * rows> aSimd(&a[0][0], std::experimental::element_aligned);
      std::experimental::fixed_size_simd<T, cols * rows> bSimd = b;

      aSimd *= bSimd;
      aSimd.copy_to(&dest[0][0], std::experimental::element_aligned);
    }

    /*
     - Multiply a matrix by a scalar, storing the result in the same matrix
       - The scalar and elements must have the same type
    */
    template <typename T, unsigned int cols, unsigned int rows>
              requires validMatrix<T, cols, rows>
    void multiply(Mat<T, cols, rows>& a, T b) {
      multiply(a, b, a);
    }

    //TODO: Implement with <simd>
    //Calculate the determinant for a square matrix
    template <typename T, unsigned int size>
              requires validMatrix<T, size, size>
    T determinant(const Mat<T, size, size>& a) {
      glm::mat<size, size, T, glm::defaultp> aMat;

      std::memcpy(glm::value_ptr(aMat), &a[0], sizeof(Mat<T, size, size>));

      return (T)glm::determinant(aMat);
    }

    //TODO: Implement with <simd>
    /*
     - Calculate the inverse of a square matrix, storing the result in dest
     - This doesn't check for invertibility first
    */
    template <typename T, unsigned int size>
              requires validMatrix<T, size, size>
    void inverse(const Mat<T, size, size>& a, Mat<T, size, size>& dest) {
      glm::mat<size, size, T, glm::defaultp> aMat;
      glm::mat<size, size, T, glm::defaultp> destMat;

      std::memcpy(glm::value_ptr(aMat), &a[0], sizeof(Mat<T, size, size>));

      destMat = glm::inverse(aMat);
      std::memcpy(&dest[0], glm::value_ptr(destMat), sizeof(Mat<T, size, size>));
    }

    //TODO: Implement with <simd>
    /*
     - Calculate a rotation matrix from an existing matrix, an angle in radians and an axis
     - Store the result in dest
     - The axis to rotate around must be normalised
    */
    template <typename T> requires validMatrix<T, 4, 4> && validVector<T, 3>
    void rotate(const Mat<T, 4, 4>& a, T angle, const Vec<T, 3> b, Mat<T, 4, 4>& dest) {
      glm::mat<4, 4, T, glm::defaultp> aMat;
      glm::vec<3, T, glm::defaultp> bVec;
      glm::mat<4, 4, T, glm::defaultp> destMat;

      std::memcpy(glm::value_ptr(aMat), &a[0], sizeof(Mat<T, 4, 4>));
      std::memcpy(glm::value_ptr(bVec), &b[0], sizeof(Vec<T, 3>));

      destMat = glm::rotate(aMat, angle, bVec);
      std::memcpy(&dest[0], glm::value_ptr(destMat), sizeof(Mat<T, 4, 4>));
    }

    /*
     - Calculate a rotation matrix from an existing matrix, an angle in radians and an axis
     - Store the result in the same matrix
     - The axis to rotate around must be normalised
    */
    template <typename T> requires validMatrix<T, 4, 4> && validVector<T, 3>
    void rotate(Mat<T, 4, 4>& a, T angle, const Vec<T, 3> b) {
      rotate(a, angle, b, a);
    }

    //TODO: Implement with <simd>
    /*
     - Calculate a scale matrix from an existing matrix and a vector of scaling ratios
     - Store the result in dest
    */
    template <typename T> requires validMatrix<T, 4, 4> && validVector<T, 3>
    void scale(const Mat<T, 4, 4>& a, const Vec<T, 3> b, Mat<T, 4, 4>& dest) {
      glm::mat<4, 4, T, glm::defaultp> aMat;
      glm::vec<3, T, glm::defaultp> bVec;
      glm::mat<4, 4, T, glm::defaultp> destMat;

      std::memcpy(glm::value_ptr(aMat), &a[0], sizeof(Mat<T, 4, 4>));
      std::memcpy(glm::value_ptr(bVec), &b[0], sizeof(Vec<T, 3>));

      destMat = glm::scale(aMat, bVec);
      std::memcpy(&dest[0], glm::value_ptr(destMat), sizeof(Mat<T, 4, 4>));
    }

    /*
     - Calculate a scale matrix from an existing matrix and a vector of scaling ratios
     - Store the result in the same matrix
    */
    template <typename T> requires validMatrix<T, 4, 4> && validVector<T, 3>
    void scale(Mat<T, 4, 4>& a, const Vec<T, 3> b) {
      scale(a, b, a);
    }

    //TODO: Implement with <simd>
    /*
     - Calculate a translation matrix from an existing matrix and a translation vector
     - Store the result in dest
    */
    template <typename T> requires validMatrix<T, 4, 4> && validVector<T, 3>
    void translate(const Mat<T, 4, 4>& a, const Vec<T, 3> b, Mat<T, 4, 4>& dest) {
      glm::mat<4, 4, T, glm::defaultp> aMat;
      glm::vec<3, T, glm::defaultp> bVec;
      glm::mat<4, 4, T, glm::defaultp> destMat;

      std::memcpy(glm::value_ptr(aMat), &a[0], sizeof(Mat<T, 4, 4>));
      std::memcpy(glm::value_ptr(bVec), &b[0], sizeof(Vec<T, 3>));

      destMat = glm::translate(aMat, bVec);
      std::memcpy(&dest[0], glm::value_ptr(destMat), sizeof(Mat<T, 4, 4>));
    }

    /*
     - Calculate a translation matrix from an existing matrix and a translation vector
     - Store the result in the same matrix
    */
    template <typename T> requires validMatrix<T, 4, 4> && validVector<T, 3>
    void translate(Mat<T, 4, 4>& a, const Vec<T, 3> b) {
      translate(a, b, a);
    }
  }

  //Utility / support functions
  inline namespace maths {
    template<typename T, unsigned int cols, unsigned int rows>
             requires validMatrix<T, cols, rows>
    std::string formatMatrix(const Mat<T, cols, rows>& matrix) {
      std::string result;
      for (unsigned int row = 0; row < rows; row++) {
        if (row != 0) {
          result += "\n";
        }

        for (unsigned int col = 0; col < cols; col++) {
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
