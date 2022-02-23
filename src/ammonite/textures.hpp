#ifndef TEXTURE
#define TEXTURE

#include <GL/glew.h>

namespace ammonite {
  namespace textures {
    GLuint loadTexture(const char* texturePath, bool* externalSuccess);
    void deleteTexture(GLuint textureId);
  }
}

#endif
