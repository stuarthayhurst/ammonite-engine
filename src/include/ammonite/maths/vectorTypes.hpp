#ifndef AMMONITEVECTORTYPES
#define AMMONITEVECTORTYPES

#include <type_traits>

namespace ammonite {
  //Definitions, concepts and constructors for vector types
  inline namespace maths {
    //Allowed vector element types
    template <typename T>
    concept vectorType = std::is_arithmetic_v<T> && (std::is_floating_point_v<T> || sizeof(T) >= 4);

    //Allowed vector sizes
    template <unsigned int size>
    concept vectorSize = size >= 2 && size <= 4;

    //Allowed vector element type and size combinations
    template <typename T, unsigned int size>
    concept validVector = vectorType<T> && vectorSize<size>;


    /*
     - Treat a typed, fixed-size block of memory as a vector
       - Since this is a raw array, it's passed by reference by default
     - These should always be passed as references, to preserve size information
       - If you specifically want a reference, leave it as one
       - If you didn't need a reference, make it a const&
       - If you specifically don't want a reference, make it a const& and copy() to an intermediate
     - In-place operations are slower than using an intermediate local variable,
       then copying back in the final operation
     - For references (a, b, c) and local variable x:
       - Prefer "add(a, b, x); add(x, b, c)" to "add(a, b, a); add(a, b, c)"
         - However, this means "a" won't be modified
       - Prefer "add(a, b, x); add(x, b, a)" to "add(a, b, a); add(a, b, a)"
    */
    template <typename T, unsigned int size> requires validVector<T, size>
    using Vec = T[size];

    /*
     - Access elements of a Vec using named attributes as references
     - Since the elements are references, copying this will not copy the vector,
       only the view
    */
    template <typename T, unsigned int size> requires validVector<T, size>
    struct NamedVec;

    //NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
    template <typename T> requires vectorType<T>
    struct NamedVec<T, 2> {
      T& x;
      T& y;

      NamedVec(Vec<T, 2>& vector): x(vector[0]), y(vector[1]) {};
    };

    template <typename T> requires vectorType<T>
    struct NamedVec<T, 3> {
      T& x;
      T& y;
      T& z;

      NamedVec(Vec<T, 3>& vector): x(vector[0]), y(vector[1]), z(vector[2]) {};
    };

    template <typename T> requires vectorType<T>
    struct NamedVec<T, 4> {
      T& x;
      T& y;
      T& z;
      T& w;

      NamedVec(Vec<T, 4>& vector): x(vector[0]), y(vector[1]), z(vector[2]), w(vector[3]) {};
    };
    //NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
  }
}

#endif
