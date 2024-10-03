#ifndef INTERNALSHADERS
#define INTERNALSHADERS

/* Internally exposed header:
 - Allow window manager to prompt checking for cache support
 - Allow loading shaders by path or directory
*/

#include <string>

namespace ammonite {
  namespace shaders {
    namespace internal {
      int createProgram(std::string* shaderPaths, unsigned int shaderCount,
                        bool* externalSuccess);
      int loadDirectory(const char* directoryPath, bool* externalSuccess);
      void updateCacheSupport();
    }
  }
}

#endif
