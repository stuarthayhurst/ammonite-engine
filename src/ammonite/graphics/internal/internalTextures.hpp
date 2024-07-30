#ifndef INTERNALTEXTURE
#define INTERNALTEXTURE

/* Internally exposed header:
 - Expose texture handling methods internally
*/

#include <GL/glew.h>

namespace ammonite {
  namespace textures {
    namespace internal {
      GLuint createTexture(int width, int height, unsigned char* data, GLenum dataFormat,
                           GLenum textureFormat, int mipmapLevels, bool* externalSuccess);
      GLuint loadTexture(const char* texturePath, bool srgbTexture, bool* externalSuccess);
      void deleteTexture(GLuint textureId);

      bool getTextureFormat(int channels, bool srgbTexture, GLenum* textureFormat,
                            GLenum* dataFormat);
      int calculateMipmapLevels(int width, int height);
    }
  }
}

#endif
