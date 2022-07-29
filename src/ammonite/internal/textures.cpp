#include <iostream>
#include <string>
#include <map>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <GL/glew.h>

#include "../utils/logging.hpp"

#include "internalDebug.hpp"

namespace ammonite {
  namespace {
    struct TextureInfo {
      GLuint textureId;
      int refCount = 1;
    };

    std::map<std::string, TextureInfo> textureTrackerMap;
    std::map<GLuint, std::string> textureIdNameMap;
  }

  namespace textures {
    void deleteTexture(GLuint textureId) {
      //Check the texture has been loaded, and get a textureName
      std::string textureName;
      if (textureIdNameMap.find(textureId) != textureIdNameMap.end()) {
        textureName = textureIdNameMap[textureId];
      } else {
        return;
      }

      TextureInfo* textureInfo = &textureTrackerMap[textureName];

      //Decrease the reference counter
      textureInfo->refCount--;

      //If texture is now unused, delete the buffer and tracker elements
      if (textureInfo->refCount < 1) {
        glDeleteTextures(1, &textureId);
        textureTrackerMap.erase(textureName);
        textureIdNameMap.erase(textureId);
      }
    }

    bool getTextureFormat(int nChannels, bool srgbTexture, GLenum* internalFormat, GLenum* dataFormat) {
      if (nChannels == 3) {
        *dataFormat = GL_RGB;
        if (srgbTexture) {
          *internalFormat = GL_SRGB8;
        } else {
          *internalFormat = GL_RGB8;
        }
      } else if (nChannels == 4) {
        *dataFormat = GL_RGBA;
        if (srgbTexture) {
          *internalFormat = GL_SRGB8_ALPHA8;
        } else {
          *internalFormat = GL_RGBA8;
        }
      } else {
        return false;
      }
      return true;
    }

    GLuint loadTexture(const char* texturePath, bool srgbTexture, bool* externalSuccess) {
      //Check if texture has already been loaded
      std::string textureString = std::string(texturePath);
      if (textureTrackerMap.find(textureString) != textureTrackerMap.end()) {
        TextureInfo* textureInfo = &textureTrackerMap[texturePath];

        textureInfo->refCount++;
        return textureInfo->textureId;
      }

      //Read image data
      int width, height, nChannels;
      unsigned char* data = stbi_load(texturePath, &width, &height, &nChannels, 0);

      if (!data) {
        std::cerr << ammonite::utils::warning << "Failed to load texture '" << texturePath << "'" << std::endl;
        *externalSuccess = false;
        return 0;
      }

      //Create a texture
      GLuint textureId;
      glCreateTextures(GL_TEXTURE_2D, 1, &textureId);

      //Decide the format of the texture and data
      GLenum internalFormat;
      GLenum dataFormat;
      if (!getTextureFormat(nChannels, srgbTexture, &internalFormat, &dataFormat)) {
        std::cerr << ammonite::utils::warning << "Failed to load texture '" << texturePath << "'" << std::endl;
        glDeleteTextures(1, &textureId);
        *externalSuccess = false;
        return 0;
      }

      //Create and fill immutable storage for the texture
      int mipmapLevels = std::floor(std::log2(std::max(width, height))) + 1;
      glTextureStorage2D(textureId, mipmapLevels, internalFormat, width, height);
      glTextureSubImage2D(textureId, 0, 0, 0, width, height, dataFormat, GL_UNSIGNED_BYTE, data);

      //When magnifying the image, use linear filtering
      glTextureParameteri(textureId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      //When minifying the image, use a linear blend of two mipmaps
      glTextureParameteri(textureId, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      //Generate mipmaps
      glGenerateTextureMipmap(textureId);

      //Release the image data
      stbi_image_free(data);

      //Save texture's info to textureTracker
      TextureInfo currentTexture;
      currentTexture.textureId = textureId;
      textureTrackerMap[textureString] = currentTexture;

      textureIdNameMap[textureId] = textureString;
      return textureId;
    }

    void copyTexture(GLuint textureId) {
      //Increase reference count on given texture, if it exists
      if (textureIdNameMap.find(textureId) != textureIdNameMap.end()) {
        textureTrackerMap[textureIdNameMap[textureId]].refCount++;
      }
    }
  }
}
