#ifndef AMMONITEVISIBILITY
#define AMMONITEVISIBILITY

#if __has_cpp_attribute(visibility)
  #define AMMONITE_EXPOSED __attribute__ ((visibility ("default")))
#else
  #define AMMONITE_EXPOSED
#endif

#if __has_cpp_attribute(visibility)
  #define AMMONITE_INTERNAL __attribute__ ((visibility ("hidden")))
#else
  #define AMMONITE_INTERNAL
#endif

#endif
