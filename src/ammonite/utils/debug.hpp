#ifndef DEBUGHEADER
#define DEBUGHEADER

//Output sent to ammoniteInternalDebug will disappear unless DEBUG is set
#ifdef DEBUG
  #include "logging.hpp"
  extern ammonite::utils::OutputHelper ammoniteInternalDebug;
#else
  #define ammoniteInternalDebug \
  if(false) std::cout
#endif

#ifdef DEBUG
  #include <iostream>
#endif

namespace ammonite {
  namespace utils {
    namespace debug {
      void enableDebug();
      void printDriverInfo();
    }
  }
}

#endif
