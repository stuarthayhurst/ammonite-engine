#include <iostream>

extern "C" {
  #include <GL/glew.h>
}

#include "extensions.hpp"

#include "../utils/debug.hpp"

namespace ammonite {
  namespace graphics {
    namespace internal {
      bool checkExtension(const char* extension, const char* version) {
        if (glewIsSupported(extension) == GL_TRUE || glewIsSupported(version) == GL_TRUE) {
          //Extension supported, either explicitly or by version
          ammoniteInternalDebug << extension << " supported (" << version << ")" << std::endl;
          return true;
        }

        //Extension unsupported
        ammoniteInternalDebug << extension << " unsupported (" << version << ")" << std::endl;
        return false;
      }

      //Allow checking for extensions without a fallback version
      bool checkExtension(const char* extension) {
        if (glewIsSupported(extension) == GL_TRUE) {
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
