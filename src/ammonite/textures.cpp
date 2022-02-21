#include <iostream>
#include <vector>
#include <string>

#include <stb/stb_image.h>
#include <GL/glew.h>

namespace ammonite {
  namespace textures {
    struct textureInfo {
      GLuint textureId;
      std::string textureName;
      int refCount = 1;
    };

    std::vector<textureInfo> textureTracker(0);
  }

  namespace textures {
    void deleteTexture(GLuint textureId) {
      //Find the textureInfo in textureTracker
      for (long unsigned int i = 0; i < textureTracker.size(); i++) {
        if (textureTracker[i].textureId == textureId) {
          //Decrease the reference count
          textureTracker[i].refCount -= 1;

          //If nothing is using the texture, delete it
          if (textureTracker[i].refCount < 1) {
            glDeleteTextures(1, &textureId);
            textureTracker.erase(std::next(textureTracker.begin(), i));
          }

          return;
        }
      }
    }

    GLuint loadTexture(const char* texturePath, bool* externalSuccess) {
      int width, height, bpp;
      unsigned char* data;

      //Check if texture is already loaded (Ridiculous datatype to match textureTracker.size())
      for (long unsigned int i = 0; i < textureTracker.size(); i++) {
        if (textureTracker[i].textureName == std::string(texturePath)) {
          textureTracker[i].refCount += 1;
          return textureTracker[i].textureId;
        }
      }

      //Read image data
      data = stbi_load(texturePath, &width, &height, &bpp, 3);

      if (!data) {
        std::cerr << "Failed to load texture '" << texturePath << "'" << std::endl;
        *externalSuccess = false;
        return 0;
      }

      //Create a texture
      GLuint textureId;
      glGenTextures(1, &textureId);

      //Select and pass texture to OpenGL
      glBindTexture(GL_TEXTURE_2D, textureId);
      if (bpp == 3) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
      } else if (bpp == 4) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
      } else {
        std::cerr << "Failed to load texture '" << texturePath << "'" << std::endl;
        glDeleteTextures(1, &textureId);
        *externalSuccess = false;
        return 0;
      }

      //Release the image data
      stbi_image_free(data);

      //When magnifying the image, use linear filtering
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      //When minifying the image, use a linear blend of two mipmaps
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      //Generate mipmaps
      glGenerateMipmap(GL_TEXTURE_2D);

      //Save texture's info to textureTracker
      textureInfo currentTexture;
      currentTexture.textureId = textureId;
      currentTexture.textureName = std::string(texturePath);
      textureTracker.push_back(currentTexture);

      glBindTexture(GL_TEXTURE_2D, 0);
      return textureId;
    }
  }
}
