#ifndef AMMONITEVECTOR
#define AMMONITEVECTOR

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <string>
#include <type_traits>

//TODO: Convert to <simd> with C++26, drop header filter regex
//NOLINTNEXTLINE(misc-include-cleaner)
#include <experimental/simd>

#include "vectorTypes.hpp"

namespace ammonite {
  //Maths operations
  inline namespace maths {
    /*
     - Return the address of the first element
     - Guaranteed to be the same as the vector, with a different type
    */
    template <typename T, unsigned int size>
              requires validVector<T, size>
    constexpr T* data(Vec<T, size>& a) {
      return &a[0];
    }

    /*
     - Return the address of the first element
     - Guaranteed to be the same as the vector, with a different type
     - const version of the above
    */
    template <typename T, unsigned int size>
              requires validVector<T, size>
    constexpr const T* data(const Vec<T, size>& a) {
      return &a[0];
    }

    //Copy from src to dest, using the size of the smaller vector as the size of the copy
    template <typename T, unsigned int sizeA, unsigned int sizeB>
              requires validVector<T, sizeA> && validVector<T, sizeB>
    constexpr void copy(const Vec<T, sizeA>& src, Vec<T, sizeB>& dest) {
      constexpr unsigned int minSize = std::min(sizeA, sizeB);
      if consteval {
        //Slower, constexpr-friendly copy
        std::copy(&src[0], &src[minSize], &dest[0]);
      } else {
        std::memcpy(&dest[0], &src[0], minSize * sizeof(src[0]));
      }
    }

    /*
     - Copy from src to dest, using the size of the smaller vector as the size of the copy
     - Additionally, cast each element during the copy
    */
    template <typename T, unsigned int sizeA, typename S, unsigned int sizeB>
              requires validVector<T, sizeA> && validVector<S, sizeB>
    constexpr void copyCast(const Vec<T, sizeA>& src, Vec<S, sizeB>& dest) {
      if constexpr (std::is_same_v<T, S>) {
        //Faster runtime copies for equal types
        ammonite::copy(src, dest);
      } else {
        constexpr unsigned int minSize = std::min(sizeA, sizeB);
        std::copy(&src[0], &src[minSize], &dest[0]);
      }
    }

    //Return true if two vectors of the same size and type have identical elements
    template <typename T, unsigned int size> requires validVector<T, size>
    constexpr bool equal(const Vec<T, size>& a, const Vec<T, size>& b) {
      //NOLINTBEGIN(readability-else-after-return)
      if consteval {
        //Slower, constexpr-friendly equality check
        for (unsigned int i = 0; i < size; i++) {
          if (a[i] != b[i]) {
            return false;
          }
        }

        return true;
      } else {
        return (std::memcmp(&a[0], &b[0], sizeof(a)) == 0);
      }
      //NOLINTEND(readability-else-after-return)
    }

    //Set every element of the vector to a scalar
    template <typename T, unsigned int size> requires validVector<T, size>
    constexpr void set(Vec<T, size>& a, T b) {
      for (unsigned int i = 0; i < size; i++) {
        a[i] = b;
      }
    }

    //Set elements of vector a to the elements of vector b and a scalar
    template <typename T, unsigned int size>
              requires validVector<T, size> && validVector<T, size - 1>
    constexpr void set(Vec<T, size>& a, const Vec<T, size - 1>& b, T c) {
      copy(b, a);
      a[size - 1] = c;
    }

    //Individually set each element of vector a of length 4 to scalars
    template <typename T> requires validVector<T, 4>
    constexpr void set(Vec<T, 4>& a, T b, T c, T d, T e) {
      a[0] = b;
      a[1] = c;
      a[2] = d;
      a[3] = e;
    }

    //Individually set each element of vector a of length 3 to scalars
    template <typename T> requires validVector<T, 3>
    constexpr void set(Vec<T, 3>& a, T b, T c, T d) {
      a[0] = b;
      a[1] = c;
      a[2] = d;
    }

    //Individually set each element of vector a of length 2 to scalars
    template <typename T> requires validVector<T, 2>
    constexpr void set(Vec<T, 2>& a, T b, T c) {
      a[0] = b;
      a[1] = c;
    }

    //Add two vectors of the same size and type, storing the result in dest
    template <typename T, unsigned int size> requires validVector<T, size>
    void add(const Vec<T, size>& a, const Vec<T, size>& b, Vec<T, size>& dest) {
      std::experimental::fixed_size_simd<T, size> aSimd(&a[0], std::experimental::element_aligned);
      std::experimental::fixed_size_simd<T, size> bSimd(&b[0], std::experimental::element_aligned);

      aSimd += bSimd;
      aSimd.copy_to(&dest[0], std::experimental::element_aligned);
    }

    //Add two vectors of the same size and type, storing the result in the first vector
    template <typename T, unsigned int size> requires validVector<T, size>
    void add(Vec<T, size>& a, const Vec<T, size>& b) {
      add(a, b, a);
    }

    /*
     - Add a scalar to each element of a vector, storing the result in dest
       - The scalar and elements must have the same type
    */
    template <typename T, unsigned int size> requires validVector<T, size>
    void add(const Vec<T, size>& a, T b, Vec<T, size>& dest) {
      std::experimental::fixed_size_simd<T, size> aSimd(&a[0], std::experimental::element_aligned);
      std::experimental::fixed_size_simd<T, size> bSimd = b;

      aSimd += bSimd;
      aSimd.copy_to(&dest[0], std::experimental::element_aligned);
    }

    /*
     - Add a scalar to each element of a vector, storing the result in the same vector
       - The scalar and elements must have the same type
    */
    template <typename T, unsigned int size> requires validVector<T, size>
    void add(Vec<T, size>& a, T b) {
      add(a, b, a);
    }

    //Subtract vector b from vector a of the same size and type, storing the result in dest
    template <typename T, unsigned int size> requires validVector<T, size>
    void sub(const Vec<T, size>& a, const Vec<T, size>& b, Vec<T, size>& dest) {
      std::experimental::fixed_size_simd<T, size> aSimd(&a[0], std::experimental::element_aligned);
      std::experimental::fixed_size_simd<T, size> bSimd(&b[0], std::experimental::element_aligned);

      aSimd -= bSimd;
      aSimd.copy_to(&dest[0], std::experimental::element_aligned);
    }

    //Subtract vector b from vector a of the same size and type, storing the result in vector a
    template <typename T, unsigned int size> requires validVector<T, size>
    void sub(Vec<T, size>& a, const Vec<T, size>& b) {
      sub(a, b, a);
    }

    /*
     - Subtract a scalar from each element of a vector, storing the result in dest
       - The scalar and elements must have the same type
    */
    template <typename T, unsigned int size> requires validVector<T, size>
    void sub(const Vec<T, size>& a, T b, Vec<T, size>& dest) {
      std::experimental::fixed_size_simd<T, size> aSimd(&a[0], std::experimental::element_aligned);
      std::experimental::fixed_size_simd<T, size> bSimd = b;

      aSimd -= bSimd;
      aSimd.copy_to(&dest[0], std::experimental::element_aligned);
    }

    /*
     - Subtract a scalar from each element of a vector, storing the result in the same vector
       - The scalar and elements must have the same type
    */
    template <typename T, unsigned int size> requires validVector<T, size>
    void sub(Vec<T, size>& a, T b) {
      sub(a, b, a);
    }

    /*
     - Multiply each element of a vector by a scalar, storing the result in dest
       - The scalar and elements must have the same type
    */
    template <typename T, unsigned int size> requires validVector<T, size>
    void scale(const Vec<T, size>& a, T b, Vec<T, size>& dest) {
      std::experimental::fixed_size_simd<T, size> aSimd(&a[0], std::experimental::element_aligned);
      std::experimental::fixed_size_simd<T, size> bSimd = b;

      aSimd *= bSimd;
      aSimd.copy_to(&dest[0], std::experimental::element_aligned);
    }

    /*
     - Multiply each element of a vector by a scalar, storing the result in the same vector
       - The scalar and elements must have the same type
    */
    template <typename T, unsigned int size> requires validVector<T, size>
    void scale(Vec<T, size>& a, T b) {
      scale(a, b, a);
    }

    /*
     - Divide each element of a vector by a scalar, storing the result in dest
       - The scalar and elements must have the same type
    */
    template <typename T, unsigned int size> requires validVector<T, size>
    void divide(const Vec<T, size>& a, T b, Vec<T, size>& dest) {
      std::experimental::fixed_size_simd<T, size> aSimd(&a[0], std::experimental::element_aligned);
      std::experimental::fixed_size_simd<T, size> bSimd = b;

      aSimd /= bSimd;
      aSimd.copy_to(&dest[0], std::experimental::element_aligned);
    }

    /*
     - Divide each element of a vector by a scalar, storing the result in the same vector
       - The scalar and elements must have the same type
    */
    template <typename T, unsigned int size> requires validVector<T, size>
    void divide(Vec<T, size>& a, T b) {
      divide(a, b, a);
    }

    /*
     - Flip the sign of every element of the vector, storing the result in dest
       - The vector's type must be signed
    */
    template <typename T, unsigned int size> requires validVector<T, size>
    void negate(const Vec<T, size>& a, Vec<T, size>& dest) {
      std::experimental::fixed_size_simd<T, size> aSimd(&a[0], std::experimental::element_aligned);

      aSimd = -aSimd;
      aSimd.copy_to(&dest[0], std::experimental::element_aligned);
    }

    /*
     - Flip the sign of every element of the vector, storing the result in the same vector
       - The vector's type must be signed
    */
    template <typename T, unsigned int size> requires validVector<T, size>
    void negate(Vec<T, size>& a) {
      negate(a, a);
    }

    /*
     - Normalise a vector, storing the result in dest
       - Intermediate calculations are done with the element's type
       - This may give strange results for integral types
    */
    template <typename T, unsigned int size> requires validVector<T, size>
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
    template <typename T, unsigned int size> requires validVector<T, size>
    void normalise(Vec<T, size>& a) {
      normalise(a, a);
    }

    //Calculate the dot product a vector
    template <typename T, unsigned int size> requires validVector<T, size>
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
    template <typename T, unsigned int size> requires validVector<T, size>
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
    template <typename T, unsigned int size> requires validVector<T, size>
    T distance(const Vec<T, size>& a, const Vec<T, size>& b) {
      std::experimental::fixed_size_simd<T, size> aSimd(&a[0], std::experimental::element_aligned);
      std::experimental::fixed_size_simd<T, size> bSimd(&b[0], std::experimental::element_aligned);

      std::experimental::fixed_size_simd<T, size> cSimd = bSimd - aSimd;

      return (T)std::sqrt(std::experimental::reduce(cSimd * cSimd, std::plus{}));
    }
  }

  //Utility / support functions
  inline namespace maths {
    template<typename T, unsigned int size> requires validVector<T, size>
    std::string formatVector(const Vec<T, size>& vector) {
      std::string result;
      for (unsigned int i = 0; i < size; i++) {
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
