#ifndef INTERNALSHADERVALIDATOR
#define INTERNALSHADERVALIDATOR

/* Internally exposed header:
 - Allow shader caches to be validated
*/

#include <cstddef>
#include <string>

#include <GL/glew.h>

namespace ammonite {
  namespace shaders {
    namespace internal {
      struct CacheInfo {
        int fileCount;
        std::string* filePaths;
        GLenum binaryFormat;
        GLsizei binaryLength;
      };

      bool validateCache(unsigned char* data, std::size_t size, void* userPtr);
    }
  }
}

#endif
