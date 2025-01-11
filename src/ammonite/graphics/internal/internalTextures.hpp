#ifndef INTERNALTEXTURE
#define INTERNALTEXTURE

/* Internally exposed header:
 - Expose texture handling methods internally
*/

#include <string>

#include <GL/glew.h>

namespace ammonite {
  namespace textures {
    namespace internal {
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
