#ifndef TEXTURE
#define TEXTURE

#include <GL/glew.h>

namespace ammonite {
  namespace textures {
    bool getTextureFormat(int nChannels, bool srgbTexture, GLenum* internalFormat, GLenum* dataFormat);
    GLuint loadTexture(const char* texturePath, bool srgbTexture, bool* externalSuccess);
    void deleteTexture(GLuint textureId);
    void copyTexture(int textureId);
  }
}

#endif
