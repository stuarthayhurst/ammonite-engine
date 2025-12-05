#ifndef AMMONITEVISIBILITY
#define AMMONITEVISIBILITY

#if __has_cpp_attribute(visibility)
  #define AMMONITE_EXPOSED __attribute__ ((visibility ("default")))
  #define AMMONITE_INTERNAL __attribute__ ((visibility ("hidden")))
#else
  #define AMMONITE_EXPOSED
  #define AMMONITE_INTERNAL
  #error "Can't control symbol visibility, expect problems"
#endif

#endif
