#ifndef TESTTEMPLATESUPPORT
#define TESTTEMPLATESUPPORT

#include <cmath>
#include <limits>
#include <type_traits>

#include <ammonite/ammonite.hpp>

template <typename T>
T randomScalar(double limit = 10000.0) {
  if constexpr (std::is_unsigned_v<T>) {
    return (T)ammonite::utils::randomUInt(std::numeric_limits<T>::max());
  } else if constexpr (std::is_integral_v<T>) {
    return (T)ammonite::utils::randomInt((T)std::sqrt(std::numeric_limits<T>::max()));
  } else {
    return (T)ammonite::utils::randomDouble((T)limit);
  }
}

template <typename T, unsigned int size>
         requires ammonite::validVector<T, size>
void randomFillVector(ammonite::Vec<T, size>& vec, double limit = 10000.0) {
  for (unsigned int i = 0; i < size; i++) {
    if constexpr (std::is_unsigned_v<T>) {
      vec[i] = (T)ammonite::utils::randomUInt(std::numeric_limits<T>::max());
    } else if constexpr (std::is_integral_v<T>) {
      vec[i] = (T)ammonite::utils::randomInt((T)std::sqrt(std::numeric_limits<T>::max() / size));
    } else {
      vec[i] = (T)ammonite::utils::randomDouble((T)limit);
    }
  }
}

template <typename T, unsigned int cols, unsigned int rows>
          requires ammonite::validMatrix<T, cols, rows>
void randomFillMatrix(ammonite::Mat<T, cols, rows>& mat, double limit = 10000.0) {
  for (unsigned int col = 0; col < cols; col++) {
    for (unsigned int row = 0; row < rows; row++) {
      mat[col][row] = (T)ammonite::utils::randomDouble((T)limit);
    }
  }
}

template <typename T> requires ammonite::validQuaternion<T>
void randomFillQuaternion(ammonite::Quat<T>& quat, double limit = 10000.0) {
  for (unsigned int i = 0; i < 4; i++) {
    quat[0][i] = (T)ammonite::utils::randomDouble((T)limit);
  }
}

template <typename T>
          requires ammonite::vectorType<T> || ammonite::matrixType<T> ||
          ammonite::validQuaternion<T>
bool roughly(T a, T b, double epsilon = 1e-5) {
  if constexpr (std::is_integral_v<T>) {
    return (a == b);
  } else {
    return std::abs(a - b) <= std::max((T)epsilon, std::max(std::abs(a), std::abs(b)) * (T)0.001);
  }
}

#endif
