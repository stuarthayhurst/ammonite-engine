#ifndef INTERNALDEBUGHEADER
#define INTERNALDEBUGHEADER

/*
 - Output sent to ammoniteInternalDebug will disappear unless AMMONITE_DEBUG is defined
 - Expressions won't even be evaluated, logging to debug is free in production
*/
#ifdef AMMONITE_DEBUG
  #include "logging.hpp"
  //NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern ammonite::utils::OutputHelper ammoniteInternalDebug;
#else
  //NOLINTNEXTLINE(misc-include-cleaner)
  #include <iostream>
  #define ammoniteInternalDebug \
  if constexpr (false) std::cout
#endif

//Include public interface
#include "../../include/ammonite/utils/debug.hpp" // IWYU pragma: export

#endif
