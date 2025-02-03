#ifndef EXTENSIONS
#define EXTENSIONS

#include "../internal.hpp"

namespace ammonite {
  namespace graphics {
    namespace AMMONITE_INTERNAL internal {
      bool checkExtension(const char* extension, const char* version);
      bool checkExtension(const char* extension);
    }
  }
}

#endif
