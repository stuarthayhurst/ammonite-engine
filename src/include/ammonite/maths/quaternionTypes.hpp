#ifndef AMMONITEQUATERNIONTYPES
#define AMMONITEQUATERNIONTYPES

#include <type_traits>

namespace ammonite {
  //Definitions, concepts and constructors for quaternion types
  inline namespace maths {
    //Allowed quaternion element types
    template <typename T>
    concept quatType = std::is_floating_point_v<T>;

    //Allowed quaternion element type and size combinations (for consistency)
    template <typename T>
    concept validQuaternion = quatType<T>;

    /*
     - Treat a typed, fixed-size block of memory as a quaternion
       - Since this is a raw array, it's passed by reference by default
       - If allocated with 'new', use 'delete []' to free it
     - The same suggestions for vectors apply here too
     - This is obviously a stupid type, but it prevents a Quat from being used
       in place of a Vec<T, 4>
       - A struct of T[4] would also work, but would default to pass by value,
         instead of pass by reference
      - If C++ ever supports strong typedefs, they should be used here
    */
    template <typename T> requires validQuaternion<T>
    using Quat = T[1][4];
  }
}

#endif
