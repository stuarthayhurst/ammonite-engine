#include <iostream>

#include <stb/stb_image.h>
#include <GL/glew.h>

namespace ammonite {
  namespace textures {
    GLuint loadTexture(const char* texturePath) {
      int width, height;
      unsigned char* data;

      //Read image data
      data = stbi_load(texturePath, &width, &height, nullptr, 3);

      //Create a texture
      GLuint textureId;
      glGenTextures(1, &textureId);

      //Select and pass texture to OpenGL
      glBindTexture(GL_TEXTURE_2D, textureId);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);

      //When magnifying the image, use linear filtering
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      //When minifying the image, use a linear blend of two mipmaps
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      //Generate mipmaps
      glGenerateMipmap(GL_TEXTURE_2D);

      return textureId;
    }
  }
}
