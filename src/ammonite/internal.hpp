#ifndef INTERNAL
#define INTERNAL

#if __has_cpp_attribute(visibility)
  #define AMMONITE_INTERNAL __attribute__ ((visibility ("hidden")))
#else
  #define AMMONITE_INTERNAL
#endif

#endif
