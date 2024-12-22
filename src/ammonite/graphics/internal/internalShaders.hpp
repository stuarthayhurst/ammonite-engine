#ifndef INTERNALSHADERS
#define INTERNALSHADERS

/* Internally exposed header:
 - Allow window manager to prompt checking for cache support
 - Allow loading shaders by path or directory
*/

#include <string>

#include <GL/glew.h>

namespace ammonite {
  namespace shaders {
    namespace internal {
      GLuint createProgram(std::string* shaderPaths, unsigned int shaderCount);
      GLuint loadDirectory(std::string directoryPath);
      void updateCacheSupport();
    }
  }
}

#endif
