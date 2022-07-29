#ifndef INTERNALDEBUG
#define INTERNALDEBUG

//Output sent to ammoniteInternalDebug will disappear unless DEBUG is set
#ifdef DEBUG
  #define ammoniteInternalDebug \
  std::cout << "DEBUG: "
#else
  #define ammoniteInternalDebug \
  if(true); else std::cout
#endif

#ifndef DEBUG
  #include <iostream>
#endif


#endif
