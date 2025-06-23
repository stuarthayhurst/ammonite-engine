#ifndef AMMONITEVECTOR
#define AMMONITEVECTOR

#include <cmath>
#include <cstring>
#include <functional>
#include <string>

//TODO: Convert to <simd> with C++26, drop header filter regex
//NOLINTNEXTLINE(misc-include-cleaner)
#include <experimental/simd>

#include "vecTypes.hpp"

namespace ammonite {
  //Maths operations
  inline namespace maths {
    //Copy from src to dest, using the size of the smaller vector as the size of the copy
    template <typename T, std::size_t sizeA, std::size_t sizeB> requires validVector<T, sizeA> && validVector<T, sizeB>
    void copy(const Vec<T, sizeA>& src, Vec<T, sizeB>& dest) {
      if constexpr (sizeA <= sizeB) {
        std::memcpy(&dest[0], &src[0], sizeof(src));
      } else {
        std::memcpy(&dest[0], &src[0], sizeof(dest));
      }
    }

    /*
     - Copy from src to dest, using the size of the smaller vector as the size of the copy
     - Additionally, cast each element during the copy
    */
    template <typename T, std::size_t sizeA, typename S, std::size_t sizeB> requires validVector<T, sizeA> && validVector<S, sizeB>
    constexpr void copyCast(const Vec<T, sizeA>& src, Vec<S, sizeB>& dest) {
      if constexpr (sizeA <= sizeB) {
        std::copy(&src[0], &src[sizeA], &dest[0]);
      } else {
        std::copy(&src[0], &src[sizeB], &dest[0]);
      }
    }

    /*
     - Return true if two vectors of the same size and type have identical elements
     - This must not be used if the vectors overlap
    */
    template <typename T, std::size_t size> requires validVector<T, size>
    bool equal(const Vec<T, size>& a, const Vec<T, size>& b) {
      return (std::memcmp(&a[0], &b[0], sizeof(a)) == 0);
    }

    //Add two vectors of the same size and type, storing the result in dest
    template <typename T, std::size_t size> requires validVector<T, size>
    void add(const Vec<T, size>& a, const Vec<T, size>& b, Vec<T, size>& dest) {
      std::experimental::fixed_size_simd<T, size> aSimd(&a[0], std::experimental::element_aligned);
      std::experimental::fixed_size_simd<T, size> bSimd(&b[0], std::experimental::element_aligned);

      aSimd += bSimd;
      aSimd.copy_to(&dest[0], std::experimental::element_aligned);
    }

    //Add two vectors of the same size and type, storing the result in the first vector
    template <typename T, std::size_t size> requires validVector<T, size>
    void add(Vec<T, size>& a, const Vec<T, size>& b) {
      add(a, b, a);
    }

    /*
     - Add a constant to each element of a vector, storing the result in dest
       - The constant and elements must have the same type
    */
    template <typename T, std::size_t size> requires validVector<T, size>
    void add(const Vec<T, size>& a, T b, Vec<T, size>& dest) {
      std::experimental::fixed_size_simd<T, size> aSimd(&a[0], std::experimental::element_aligned);
      std::experimental::fixed_size_simd<T, size> bSimd = b;

      aSimd += bSimd;
      aSimd.copy_to(&dest[0], std::experimental::element_aligned);
    }

    /*
     - Add a constant to each element of a vector, storing the result in the same vector
       - The constant and elements must have the same type
    */
    template <typename T, std::size_t size> requires validVector<T, size>
    void add(Vec<T, size>& a, T b) {
      add(a, b, a);
    }

    //Subtract vector b from vector a of the same size and type, storing the result in dest
    template <typename T, std::size_t size> requires validVector<T, size>
    void sub(const Vec<T, size>& a, const Vec<T, size>& b, Vec<T, size>& dest) {
      std::experimental::fixed_size_simd<T, size> aSimd(&a[0], std::experimental::element_aligned);
      std::experimental::fixed_size_simd<T, size> bSimd(&b[0], std::experimental::element_aligned);

      aSimd -= bSimd;
      aSimd.copy_to(&dest[0], std::experimental::element_aligned);
    }

    //Subtract vector b from vector a of the same size and type, storing the result in vector a
    template <typename T, std::size_t size> requires validVector<T, size>
    void sub(Vec<T, size>& a, const Vec<T, size>& b) {
      sub(a, b, a);
    }

    /*
     - Subtract a constant from each element of a vector, storing the result in dest
       - The constant and elements must have the same type
    */
    template <typename T, std::size_t size> requires validVector<T, size>
    void sub(const Vec<T, size>& a, T b, Vec<T, size>& dest) {
      std::experimental::fixed_size_simd<T, size> aSimd(&a[0], std::experimental::element_aligned);
      std::experimental::fixed_size_simd<T, size> bSimd = b;

      aSimd -= bSimd;
      aSimd.copy_to(&dest[0], std::experimental::element_aligned);
    }

    /*
     - Subtract a constant from each element of a vector, storing the result in the same vector
       - The constant and elements must have the same type
    */
    template <typename T, std::size_t size> requires validVector<T, size>
    void sub(Vec<T, size>& a, T b) {
      sub(a, b, a);
    }

    /*
     - Multiply each element of a vector by a constant, storing the result in dest
       - The constant and elements must have the same type
    */
    template <typename T, std::size_t size> requires validVector<T, size>
    void scale(const Vec<T, size>& a, T b, Vec<T, size>& dest) {
      std::experimental::fixed_size_simd<T, size> aSimd(&a[0], std::experimental::element_aligned);
      std::experimental::fixed_size_simd<T, size> bSimd = b;

      aSimd *= bSimd;
      aSimd.copy_to(&dest[0], std::experimental::element_aligned);
    }

    /*
     - Multiply each element of a vector by a constant, storing the result in the same vector
       - The constant and elements must have the same type
    */
    template <typename T, std::size_t size> requires validVector<T, size>
    void scale(Vec<T, size>& a, T b) {
      scale(a, b, a);
    }

    /*
     - Divide each element of a vector by a constant, storing the result in dest
       - The constant and elements must have the same type
    */
    template <typename T, std::size_t size> requires validVector<T, size>
    void div(const Vec<T, size>& a, T b, Vec<T, size>& dest) {
      std::experimental::fixed_size_simd<T, size> aSimd(&a[0], std::experimental::element_aligned);
      std::experimental::fixed_size_simd<T, size> bSimd = b;

      aSimd /= bSimd;
      aSimd.copy_to(&dest[0], std::experimental::element_aligned);
    }

    /*
     - Divide each element of a vector by a constant, storing the result in the same vector
       - The constant and elements must have the same type
    */
    template <typename T, std::size_t size> requires validVector<T, size>
    void div(Vec<T, size>& a, T b) {
      div(a, b, a);
    }

    /*
     - Normalise a vector, storing the result in dest
       - Intermediate calculations are done with the element's type
       - This may give strange results for integral types
    */
    template <typename T, std::size_t size> requires validVector<T, size>
    void normalise(const Vec<T, size>& a, Vec<T, size>& dest) {
      std::experimental::fixed_size_simd<T, size> aSimd(&a[0], std::experimental::element_aligned);

      T sum = std::experimental::reduce(aSimd * aSimd, std::plus{});
      aSimd /= (T)std::sqrt(sum);

      aSimd.copy_to(&dest[0], std::experimental::element_aligned);
    }

    /*
     - Normalise a vector, storing the result in the same vector
       - Intermediate calculations are done with the element's type
       - This may give strange results for integral types
    */
    template <typename T, std::size_t size> requires validVector<T, size>
    void normalise(Vec<T, size>& a) {
      normalise(a, a);
    }

    //Calculate the dot product a vector
    template <typename T, std::size_t size> requires validVector<T, size>
    T dot(const Vec<T, size>& a, const Vec<T, size>& b) {
      std::experimental::fixed_size_simd<T, size> aSimd(&a[0], std::experimental::element_aligned);
      std::experimental::fixed_size_simd<T, size> bSimd(&b[0], std::experimental::element_aligned);

      return std::experimental::reduce(aSimd * bSimd, std::plus{});
    }

    /*
     - Calculate the cross product for two vectors of the same size and type
       - Store the result in dest
    */
    template <typename T> requires vectorType<T>
    constexpr void cross(const Vec<T, 3>& a, const Vec<T, 3>& b, Vec<T, 3>& dest) {
      //TODO: Make use of scatter / gather / permute instructions, when available
      dest[0] = (a[1] * b[2]) - (a[2] * b[1]);
      dest[1] = (a[2] * b[0]) - (a[0] * b[2]);
      dest[2] = (a[0] * b[1]) - (a[1] * b[0]);
    }

    /*
     - Calculate the length of a vector
       - Intermediate calculations are done with the element's type
       - This may give strange results for integral types
    */
    template <typename T, std::size_t size> requires validVector<T, size>
    T length(const Vec<T, size>& a) {
      std::experimental::fixed_size_simd<T, size> aSimd(&a[0], std::experimental::element_aligned);

      return (T)std::sqrt(std::experimental::reduce(aSimd * aSimd, std::plus{}));
    }

    /*
     - Calculate the distance between two vectors of the same size and type
       - Intermediate calculations are done with the element's type
       - This may give strange results for integral types
     - Logically, this is equivalent to length(b - a)
    */
    template <typename T, std::size_t size> requires validVector<T, size>
    T distance(const Vec<T, size>& a, const Vec<T, size>& b) {
      std::experimental::fixed_size_simd<T, size> aSimd(&a[0], std::experimental::element_aligned);
      std::experimental::fixed_size_simd<T, size> bSimd(&b[0], std::experimental::element_aligned);

      std::experimental::fixed_size_simd<T, size> cSimd = bSimd - aSimd;

      return (T)std::sqrt(std::experimental::reduce(cSimd * cSimd, std::plus{}));
    }
  }

  //Utility / support functions
  inline namespace maths {
    template<typename T, std::size_t size> requires validVector<T, size>
    std::string formatVector(const Vec<T, size>& vector) {
      std::string result;
      for (std::size_t i = 0; i < size; i++) {
        if (i != 0) {
          result += ", ";
        }

        result += std::to_string(vector[i]);
      }

      return result;
    }
  }
}

#endif
