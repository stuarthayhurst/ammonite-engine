#ifndef AMMONITEVISIBILITY
#define AMMONITEVISIBILITY

#if __has_attribute(visibility)
  #define AMMONITE_EXPOSED __attribute__((visibility("default")))
  #define AMMONITE_INTERNAL __attribute__((visibility("hidden")))
#elif __has_cpp_attribute(gnu::visibility)
  #define AMMONITE_EXPOSED [[gnu::visibility("default")]]
  #define AMMONITE_INTERNAL [[gnu::visibility("hidden")]]
#else
  #define AMMONITE_EXPOSED
  #define AMMONITE_INTERNAL
  #error "Can't control symbol visibility, expect problems"
#endif

#endif
