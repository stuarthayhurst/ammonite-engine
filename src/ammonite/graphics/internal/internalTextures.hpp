#ifndef INTERNALTEXTURE
#define INTERNALTEXTURE

/* Internally exposed header:
 - Expose texture handling methods internally
*/

#include <GL/glew.h>

namespace ammonite {
  namespace textures {
    namespace internal {
      bool getTextureFormat(int nChannels, bool srgbTexture, GLenum* internalFormat, GLenum* dataFormat);
      GLuint loadTexture(const char* texturePath, bool srgbTexture, bool* externalSuccess);
      void deleteTexture(GLuint textureId);
      void copyTexture(GLuint textureId);
    }
  }
}

#endif
