#ifndef TEXTURE
#define TEXTURE

namespace ammonite {
  namespace textures {
    GLuint loadTexture(const char* texturePath, bool* externalSuccess);
  }
}

#endif
