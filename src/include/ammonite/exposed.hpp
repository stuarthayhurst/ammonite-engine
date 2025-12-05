#ifndef AMMONITEEXPOSED
#define AMMONITEEXPOSED

#if __has_cpp_attribute(visibility)
  #define AMMONITE_EXPOSED __attribute__ ((visibility ("default")))
#else
  #define AMMONITE_EXPOSED
#endif

#endif
