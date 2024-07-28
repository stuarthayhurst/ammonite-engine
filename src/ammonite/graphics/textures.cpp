#include <iostream>
#include <string>
#include <map>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <GL/glew.h>

#include "../utils/logging.hpp"

namespace ammonite {
  namespace textures {
    namespace internal {
      namespace {
        struct TextureInfo {
          std::string path;
          GLuint id;
          int refCount;
        };

        std::map<GLuint, TextureInfo> idTextureMap;
        std::map<std::string, TextureInfo*> pathTexturePtrMap;
      }

      namespace {
        /*
         - Returns true if it could decide which formats to use, false otherwise
         - Writes the format of the data to dataFormat
         - Writes the format of the final texture to textureFormat
        */
        static bool decideTextureFormat(int channels, bool srgbTexture, GLenum* textureFormat,
                                        GLenum* dataFormat) {
          if (channels == 3) {
            *dataFormat = GL_RGB;
            if (srgbTexture) {
              *textureFormat = GL_SRGB8;
            } else {
              *textureFormat = GL_RGB8;
            }
          } else if (channels == 4) {
            *dataFormat = GL_RGBA;
            if (srgbTexture) {
              *textureFormat = GL_SRGB8_ALPHA8;
            } else {
              *textureFormat = GL_RGBA8;
            }
          } else {
            return false;
          }

          return true;
        }
      }

      //Temporary function to transition to new system
      bool getTextureFormat(int channels, bool srgbTexture, GLenum* textureFormat,
                            GLenum* dataFormat) {
        return decideTextureFormat(channels, srgbTexture, textureFormat, dataFormat);
      }

      /*
       - Deletes a texture created with createTexture() or loadTexture()
      */
      void deleteTexture(GLuint textureId) {
        //Fetch the texture info, if it exists
        if (!idTextureMap.contains(textureId)) {
          ammonite::utils::warning << "Not deleting texture (ID " << textureId \
                                   << "), it doesn't exist" << std::endl;
          return;
        }
        TextureInfo* textureInfoPtr = &idTextureMap[textureId];

        //Decrease the reference counter, delete the texture if now unused
        if (--textureInfoPtr->refCount <= 0) {
          //Remove the path entry
          if (textureInfoPtr->path != "") {
            pathTexturePtrMap.erase(textureInfoPtr->path);
          }

          //Delete the texture
          glDeleteTextures(1, &textureInfoPtr->id);

          //Delete the tracker entry
          idTextureMap.erase(textureId);
        }
      }

      /*
       - Create a texture from the data given, return its ID
       - This doesn't generate the mipmaps, but allocates space for them
       - externalSuccess is written to on failure
      */
      GLuint createTexture(int width, int height, unsigned char* data, GLenum dataFormat,
                           GLenum textureFormat, int mipmapLevels, bool* externalSuccess) {
        //Check texture size is within limits
        GLint maxTextureSize = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
        if (width > maxTextureSize || height > maxTextureSize) {
          ammonite::utils::warning << "Attempted to create a texture of unsupported size (" \
                                   << width << " x " << height << ")" << std::endl;

          *externalSuccess = false;
          return -1;
        }

        //Create a texture, its storage, and then fill it
        GLuint textureId;
        glCreateTextures(GL_TEXTURE_2D, 1, &textureId);
        glTextureStorage2D(textureId, mipmapLevels, textureFormat, width, height);
        glTextureSubImage2D(textureId, 0, 0, 0, width, height, dataFormat, GL_UNSIGNED_BYTE, data);

        //Add the texture to the tracker
        if (idTextureMap.contains(textureId)) {
          ammonite::utils::warning << "Texture ID (" << textureId \
                                   << ") already exists, not creating texture" << std::endl;
          *externalSuccess = false;
          return -1;
        }
        idTextureMap[textureId] = {"", textureId, 1};

        return textureId;
      }

      /*
       - Load a texture from a file and return its ID, respecting srgbTexture
       - Writes false to externalSuccess on failure
      */
      GLuint loadTexture(const char* texturePath, bool srgbTexture, bool* externalSuccess) {
        //Check if texture has already been loaded
        std::string textureString = std::string(texturePath);
        if (pathTexturePtrMap.contains(textureString)) {
          TextureInfo* textureInfo = pathTexturePtrMap[texturePath];

          textureInfo->refCount++;
          return textureInfo->id;
        }

        //Read image data
        int width, height, nChannels;
        unsigned char* data = stbi_load(texturePath, &width, &height, &nChannels, 0);
        if (data == nullptr) {
          ammonite::utils::warning << "Failed to load texture '" << texturePath << "'" \
                                   << std::endl;
          *externalSuccess = false;
          return -1;
        }

        //Decide the format of the texture and data
        GLenum textureFormat, dataFormat;
        if (!decideTextureFormat(nChannels, srgbTexture, &textureFormat, &dataFormat)) {
          ammonite::utils::warning << "Failed to load texture '" << texturePath << "'" \
                                   << std::endl;
          stbi_image_free(data);
          *externalSuccess = false;
          return -1;
        }

        //Create the texture and free the image data
        bool success = true;
        int mipmapLevels = std::floor(std::log2(std::max(width, height))) + 1;
        GLuint textureId = createTexture(width, height, data, dataFormat, textureFormat,
                                         mipmapLevels, &success);
        stbi_image_free(data);
        if (!success) {
          ammonite::utils::warning << "Failed to load texture '" << texturePath << "'" \
                                   << std::endl;
          *externalSuccess = false;
          return -1;
        }

        //Connect the texture path to the ID and info
        TextureInfo* textureInfoPtr = &idTextureMap[textureId];
        pathTexturePtrMap[textureString] = textureInfoPtr;
        textureInfoPtr->path = textureString;

        //When magnifying the image, use linear filtering
        glTextureParameteri(textureId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        //When minifying the image, use a linear blend of two mipmaps
        glTextureParameteri(textureId, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        //Generate mipmaps
        glGenerateTextureMipmap(textureId);

        return textureId;
      }
    }
  }
}
