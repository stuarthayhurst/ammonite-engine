#ifndef DEBUGHEADER
#define DEBUGHEADER

//Output sent to ammoniteInternalDebug will disappear unless DEBUG is set
#ifdef DEBUG
  #include "logging.hpp"
  //NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern ammonite::utils::OutputHelper ammoniteInternalDebug;
#else
  //NOLINTNEXTLINE(misc-include-cleaner)
  #include <iostream>
  #define ammoniteInternalDebug \
  if(false) std::cout
#endif

namespace ammonite {
  namespace utils {
    namespace debug {
      //Exported by the engine
      void enableDebug();
      void printDriverInfo();
    }
  }
}

#endif
