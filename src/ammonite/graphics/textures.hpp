#ifndef INTERNALTEXTURES
#define INTERNALTEXTURES

#include <string>

extern "C" {
  #include <epoxy/gl.h>
}

#include "../visibility.hpp"

namespace ammonite {
  namespace textures {
    namespace AMMONITE_INTERNAL internal {
      GLuint createTexture(int width, int height, unsigned char* data, GLenum dataFormat,
                           GLenum textureFormat, GLint textureLevels);
      GLuint loadTexture(const std::string& texturePath, bool flipTexture, bool srgbTexture);
      void copyTexture(GLuint textureId);
      void deleteTexture(GLuint textureId);

      GLuint loadCubemap(std::string texturePaths[6], bool flipTextures, bool srgbTextures);

      unsigned int calculateMipmapLevels(int width, int height);
    }
  }
}

#endif
