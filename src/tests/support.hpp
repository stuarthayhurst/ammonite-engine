#ifndef TESTTEMPLATESUPPORT
#define TESTTEMPLATESUPPORT

#include <cmath>
#include <limits>
#include <type_traits>

#include <ammonite/ammonite.hpp>

template <typename T, unsigned int size>
         requires ammonite::validVector<T, size>
void randomFillVector(ammonite::Vec<T, size>& vec) {
  for (unsigned int i = 0; i < size; i++) {
    if constexpr (std::is_unsigned_v<T>) {
      vec[i] = (T)ammonite::utils::randomUInt(std::numeric_limits<T>::max());
    } else if constexpr (std::is_integral_v<T>) {
      vec[i] = (T)ammonite::utils::randomInt((T)std::sqrt(std::numeric_limits<T>::max() / size));
    } else {
      vec[i] = (T)ammonite::utils::randomDouble(1000000.0f);
    }
  }
}

template <typename T, unsigned int cols, unsigned int rows>
          requires ammonite::validMatrix<T, cols, rows>
void randomFillMatrix(ammonite::Mat<T, cols, rows>& mat) {
  for (unsigned int col = 0; col < cols; col++) {
    for (unsigned int row = 0; row < rows; row++) {
      mat[col][row] = (T)ammonite::utils::randomDouble(1000000.0f);
    }
  }
}

template <typename T> requires ammonite::vectorType<T> || ammonite::matrixType<T>
bool roughly(T a, T b) {
  if constexpr (std::is_integral_v<T>) {
    return (a == b);
  } else {
    return std::abs(a - b) <= std::max((T)1e-5f, std::max(std::abs(a), std::abs(b)) * (T)0.001);
  }
}

#endif
