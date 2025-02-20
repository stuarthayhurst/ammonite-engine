#ifndef INTERNALDEBUGHEADER
#define INTERNALDEBUGHEADER

//Output sent to ammoniteInternalDebug will disappear unless DEBUG is set
#ifdef AMMONITE_DEBUG
  #include "logging.hpp"
  //NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern ammonite::utils::OutputHelper ammoniteInternalDebug;
#else
  //NOLINTNEXTLINE(misc-include-cleaner)
  #include <iostream>
  #define ammoniteInternalDebug \
  if(false) std::cout
#endif

//Include public interface
#include "../../include/ammonite/utils/debug.hpp"

#endif
