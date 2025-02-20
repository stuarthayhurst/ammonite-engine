#ifndef INTERNALTEXTURES
#define INTERNALTEXTURES

#include <string>

extern "C" {
  #include <GL/glew.h>
}

#include "../internal.hpp"

namespace ammonite {
  namespace textures {
    namespace AMMONITE_INTERNAL internal {
      GLuint createTexture(int width, int height, unsigned char* data, GLenum dataFormat,
                           GLenum textureFormat, GLint mipmapLevels);
      GLuint loadTexture(std::string texturePath, bool flipTexture, bool srgbTexture);
      void deleteTexture(GLuint textureId);

      bool getTextureFormat(int channels, bool srgbTexture, GLenum* textureFormat,
                            GLenum* dataFormat);
      unsigned int calculateMipmapLevels(int width, int height);
    }
  }
}

#endif
