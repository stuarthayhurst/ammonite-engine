#ifndef AMMONITEMATRIX
#define AMMONITEMATRIX

#include <cstring>

//TODO: Convert to <simd> with C++26, drop header filter regex
//NOLINTNEXTLINE(misc-include-cleaner)
#include <experimental/simd>

#include "matTypes.hpp"

namespace ammonite {
  //Maths operations
  inline namespace maths {
    //Copy from src to dest
    template <typename T, std::size_t cols, std::size_t rows> requires validMatrix<T, cols, rows>
    void copy(const Mat<T, cols, rows>& src, Mat<T, cols, rows>& dest) {
      std::memcpy(&dest[0], &src[0], sizeof(Mat<T, cols, rows>));
    }

    /*
     - Copy from src to dest
     - Additionally, cast each element during the copy
    */
    template <typename T, std::size_t cols, std::size_t rows,
              typename S, std::size_t m, std::size_t n>
              requires validMatrix<T, cols, rows> && validMatrix<S, cols, rows> &&
                       (m == cols) && (n == rows)
    constexpr void copyCast(const Mat<T, cols, rows>& src, Mat<S, cols, rows>& dest) {
      std::copy(&src[0], &src[cols * rows], &dest[0]);
    }
  }
}

#endif
