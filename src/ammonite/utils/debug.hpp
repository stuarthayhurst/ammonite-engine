#ifndef DEBUGHEADER
#define DEBUGHEADER

//Output sent to ammoniteInternalDebug will disappear unless DEBUG is set
#ifdef DEBUG
  #define ammoniteInternalDebug \
  std::cout << "DEBUG: "
#else
  #define ammoniteInternalDebug \
  if(true); else std::cout
#endif

#ifdef DEBUG
  #include <iostream>
#endif

#ifdef DEBUG
namespace ammonite {
  namespace utils {
    namespace debug {
      void enableDebug();
    }
  }
}
#endif

#endif
