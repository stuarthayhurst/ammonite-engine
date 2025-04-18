#ifndef INTERNALSHADERLOADER
#define INTERNALSHADERLOADER

#include <string>

#include <GL/glew.h>

#include "../internal.hpp"

namespace ammonite {
  namespace shaders {
    namespace AMMONITE_INTERNAL internal {
      GLuint createProgram(std::string* shaderPaths, unsigned int shaderCount);
      GLuint loadDirectory(const std::string& directoryPath);
      void updateCacheSupport();
    }
  }
}

#endif
