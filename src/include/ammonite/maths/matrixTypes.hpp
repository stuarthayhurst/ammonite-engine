#ifndef AMMONITEMATRIXTYPES
#define AMMONITEMATRIXTYPES

#include <type_traits>

#include "vectorTypes.hpp"

namespace ammonite {
  //Definitions, concepts and constructors for matrix types
  inline namespace maths {
    //Allowed matrix element types
    template <typename T>
    concept matrixType = std::is_arithmetic_v<T> && (std::is_floating_point_v<T> || sizeof(T) >= 4);

    //Allowed matrix dimensions
    template <unsigned int size>
    concept matrixSize = size >= 2 && size <= 4;

    //Allowed vector element type and size combinations
    template <typename T, unsigned int cols, unsigned int rows>
    concept validMatrix = matrixType<T> && matrixSize<cols> && matrixSize<rows>;


    /*
     - Treat a typed, fixed-size block of memory as a matrix
       - Since this is a raw array, it's passed by reference by default
     - These are designed for GLSL, so they're column-major
     - The same suggestions for vectors apply here too
    */
    template <typename T, unsigned int cols, unsigned int rows>
              requires validMatrix<T, cols, rows>
    using Mat = Vec<T, rows>[cols];
  }
}

#endif
