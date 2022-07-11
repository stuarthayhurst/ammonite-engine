#ifndef INTERNALTEXTURE
#define INTERNALTEXTURE

/* Internally exposed header:
 - Allow model tracker to copy textures
*/

namespace ammonite {
  namespace textures {
    void copyTexture(int textureId);
  }
}

#endif
