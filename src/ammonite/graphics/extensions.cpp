#include <iostream>

extern "C" {
  #include <epoxy/gl.h>
}

#include "extensions.hpp"

#include "../utils/debug.hpp"

namespace ammonite {
  namespace graphics {
    namespace internal {
      bool checkGlVersion(int majorVersion, int minorVersion) {
        return (epoxy_gl_version() >= ((majorVersion * 10) + minorVersion));
      }

      bool checkExtension(const char* extension, int major, int minor) {
        if (epoxy_has_gl_extension(extension) || checkGlVersion(major, minor)) {
          //Extension supported, either explicitly or by version
          ammoniteInternalDebug << extension << " supported (" << major << "." \
                                << minor << ")" << std::endl;
          return true;
        }

        //Extension unsupported
        ammoniteInternalDebug << extension << " unsupported (" << major << "." \
                              << minor << ")" << std::endl;
        return false;
      }

      //Allow checking for extensions without a fallback version
      bool checkExtension(const char* extension) {
        if (epoxy_has_gl_extension(extension)) {
          //Extension supported
          ammoniteInternalDebug << extension << " supported" << std::endl;
          return true;
        }

        //Extension unsupported
        ammoniteInternalDebug << extension << " unsupported" << std::endl;
        return false;
      }
    }
  }
}
