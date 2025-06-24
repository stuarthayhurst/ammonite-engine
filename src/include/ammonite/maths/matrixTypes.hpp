#ifndef AMMONITEMATRIXTYPES
#define AMMONITEMATRIXTYPES

#include <bit>
#include <cstddef>
#include <type_traits>

namespace ammonite {
  //Definitions, concepts and constructors for matrix types
  inline namespace maths {
    //Allowed matrix element types
    template <typename T>
    concept matrixType = std::is_floating_point_v<T> && (sizeof(T) >= 4);

    //Allowed matrix dimensions
    template <std::size_t size>
    concept matrixSize = size >= 2 && size <= 4;

    //Allowed vector element type and size combinations
    template <typename T, std::size_t cols, std::size_t rows>
    concept validMatrix = matrixType<T> && matrixSize<cols> && matrixSize<cols>;


    /*
     - Treat a typed, fixed-size block of memory as a matrix
       - Since this is a raw array, it's passed by reference by default
     - These are designed for GLSL, so they're column-major
       - Mx3 matrices get rounded up to Mx4 internally
     - The same suggestions for vectors apply here too
    */
    template <typename T, std::size_t cols, std::size_t rows> requires validMatrix<T, cols, rows>
    using Mat = T[cols][std::bit_ceil(rows)];
  }
}

#endif
