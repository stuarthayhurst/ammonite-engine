#include <iostream>
#include <string>
#include <map>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <GL/glew.h>

namespace ammonite {
  namespace {
    struct TextureInfo {
      GLuint textureId;
      int refCount = 1;
    };

    std::map<std::string, TextureInfo> textureTrackerMap;
    std::map<GLuint, std::string> idToNameMap;
  }

  namespace textures {
    void deleteTexture(GLuint textureId) {
      std::string textureName;
      //Check the texture has been loaded, and get a textureName
      if (idToNameMap.find(textureId) != idToNameMap.end()) {
        textureName = idToNameMap[textureId];
      } else {
        return;
      }

      //Decrease the reference counter
      textureTrackerMap[textureName].refCount -= 1;

      //If texture is now unused, delete the buffer and tracker elements
      if (textureTrackerMap[textureName].refCount < 1) {
        glDeleteTextures(1, &textureId);
        textureTrackerMap.erase(textureName);
        idToNameMap.erase(textureId);
      }
    }

    GLuint loadTexture(const char* texturePath, bool* externalSuccess) {
      int width, height, bpp;
      unsigned char* data;

      std::string textureString = std::string(texturePath);

      //Check if texture has already been loaded
      if (textureTrackerMap.find(textureString) != textureTrackerMap.end()) {
        textureTrackerMap[textureString].refCount += 1;
        return textureTrackerMap[textureString].textureId;
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
      TextureInfo currentTexture;
      currentTexture.textureId = textureId;
      textureTrackerMap[textureString] = currentTexture;

      idToNameMap[textureId] = textureString;

      glBindTexture(GL_TEXTURE_2D, 0);
      return textureId;
    }
  }
}
