#ifndef INTERNALEXTENSIONS
#define INTERNALEXTENSIONS

#include "../visibility.hpp"

namespace AMMONITE_INTERNAL ammonite {
  namespace graphics {
    namespace internal {
      bool checkGlVersion(int majorVersion, int minorVersion);
      bool checkExtension(const char* extension, int majorVersion, int minorVersion);
      bool checkExtension(const char* extension);
    }
  }
}

#endif
