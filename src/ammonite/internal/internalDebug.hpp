#ifndef INTERNALDEBUG
#define INTERNALDEBUG

//Output sent to ammoniteInternalDebug will disappear unless DEBUG is set
#ifndef DEBUG
  #define ammoniteInternalDebug \
  if(true); else std::cout
#else
  #define ammoniteInternalDebug \
  std::cout << "DEBUG: "
#endif

#ifndef DEBUG
  #include <iostream>
#endif


#endif
