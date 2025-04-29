#ifndef INTERNALEXTENSIONS
#define INTERNALEXTENSIONS

#include "../internal.hpp"

namespace ammonite {
  namespace graphics {
    namespace AMMONITE_INTERNAL internal {
      bool checkGlVersion(int majorVersion, int minorVersion);
      bool checkExtension(const char* extension, int majorVersion, int minorVersion);
      bool checkExtension(const char* extension);
    }
  }
}

#endif
