#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>

extern "C" {
  #include <epoxy/gl.h>
  #define STB_IMAGE_IMPLEMENTATION
  #include <stb/stb_image.h>
}

#include "textures.hpp"

#include "../maths/vector.hpp"
#include "../utils/debug.hpp"
#include "../utils/logging.hpp"

namespace ammonite {
  namespace textures {
    namespace internal {
      namespace {
        struct TextureInfo {
          GLuint id;
          unsigned int refCount = 0;
          std::string string;
          std::array<float, 3> colour;
          bool isColour;
        };

        std::map<GLuint, TextureInfo> idTextureMap;
        std::unordered_map<std::string, TextureInfo*> stringTexturePtrMap;
        std::map<std::array<float, 3>, TextureInfo*> colourTexturePtrMap;
      }

      namespace {
        /*
         - Returns true if it could decide which formats to use, false otherwise
         - Writes the format of the data to dataFormat
         - Writes the format of the final texture to textureFormat
        */
        bool decideTextureFormat(int channels, bool srgbTexture,
                                 GLenum* textureFormat, GLenum* dataFormat) {
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

      //Calculate the number of mipmaps levels to use
      unsigned int calculateMipmapLevels(int width, int height) {
        return (unsigned int)std::log2(std::max(width, height)) + 1;
      }

      /*
       - Deletes a texture created with createTexture() or load*Texture()
      */
      void deleteTexture(GLuint textureId) {
        //Fetch the texture info, if it exists
        if (!idTextureMap.contains(textureId)) {
          ammonite::utils::warning << "Not deleting texture (ID " << textureId \
                                   << "), it doesn't exist" << std::endl;
          return;
        }
        TextureInfo* const textureInfoPtr = &idTextureMap[textureId];

        //Decrease the reference counter, delete the texture if now unused
        if (--textureInfoPtr->refCount == 0) {
          //Remove the string entry
          if (!textureInfoPtr->string.empty()) {
            stringTexturePtrMap.erase(textureInfoPtr->string);
          }

          //Remove the colour entry
          if (textureInfoPtr->isColour) {
            colourTexturePtrMap.erase(textureInfoPtr->colour);
          }

          //Delete the texture
          glDeleteTextures(1, &textureInfoPtr->id);

          //Delete the tracker entry
          if (!textureInfoPtr->isColour) {
            ammoniteInternalDebug << "Deleted storage for file texture (ID " << textureId \
                                  << ", '" << textureInfoPtr->string << "')" << std::endl;
          } else {
            ammoniteInternalDebug << "Deleted storage for colour texture (ID " << textureId \
                                  << ")" << std::endl;
          }

          idTextureMap.erase(textureId);
        }
      }

      //Increase the reference count of a texture by its ID
      void copyTexture(GLuint textureId) {
        if (!idTextureMap.contains(textureId)) {
          ammonite::utils::warning << "Texture ID (" << textureId \
                                   << ") doesn't exist, not copying texture" << std::endl;
          return;
        }

        idTextureMap[textureId].refCount++;
      }

      /*
       - Create a texture from the data given, return its ID
       - This doesn't generate the mipmaps, but allocates space for them (textureLevels)
       - Makes no attempt at caching / deduplicating it
       - Returns 0 on failure
      */
      GLuint createTexture(int width, int height, unsigned char* data, GLenum dataFormat,
                           GLenum textureFormat, GLint textureLevels) {
        //Check texture size is within limits
        GLint maxTextureSize = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
        if (width > maxTextureSize || height > maxTextureSize) {
          ammonite::utils::warning << "Attempted to create a texture of unsupported size (" \
                                   << width << " x " << height << ")" << std::endl;
          return 0;
        }

        //Create a texture, its storage, and then fill it
        GLuint textureId = 0;
        glCreateTextures(GL_TEXTURE_2D, 1, &textureId);
        glTextureStorage2D(textureId, textureLevels, textureFormat, width, height);
        glTextureSubImage2D(textureId, 0, 0, 0, width, height, dataFormat, GL_UNSIGNED_BYTE, data);

        //Add the texture to the tracker
        if (idTextureMap.contains(textureId)) {
          ammonite::utils::warning << "Texture ID (" << textureId \
                                   << ") already exists, not creating texture" << std::endl;
          return 0;
        }
        idTextureMap[textureId] = {textureId, 1, "", {0}, false};

        return textureId;
      }

      /*
       - Load a texture from a colour and return its ID
       - Caches / deduplicates identical solid colour textures
       - Returns 0 on failure
      */
      GLuint loadSolidTexture(const ammonite::Vec<float, 3>& colour) {
        //Copy the colour into a standard type as a key
        std::array<float, 3> colourArray;
        std::memcpy(colourArray.data(), ammonite::data(colour), sizeof(colour));

        //Check if this colour has already been loaded
        if (colourTexturePtrMap.contains(colourArray)) {
          TextureInfo* const textureInfo = colourTexturePtrMap[colourArray];

          textureInfo->refCount++;
          return textureInfo->id;
        }

        //Decide the format of the texture and data
        GLenum textureFormat = 0, dataFormat = 0;
        if (!decideTextureFormat(3, false, &textureFormat, &dataFormat)) {
          ammonite::utils::warning << "Failed to load texture from colour" << std::endl;
          return 0;
        }

        //Convert the colour into the required format
        ammonite::Vec<float, 3> scaledColour = {0};
        ammonite::scale(colour, 255.0f, scaledColour);
        unsigned char data[3] = {
          (unsigned char)scaledColour[0],
          (unsigned char)scaledColour[1],
          (unsigned char)scaledColour[2]
        };

        //Create the texture and free the image data
        const unsigned int mipmapLevels = calculateMipmapLevels(1, 1);
        const GLuint textureId = createTexture(1, 1, data, dataFormat, textureFormat,
                                               (GLint)mipmapLevels);
        if (textureId == 0) {
          ammonite::utils::warning << "Failed to load texture from colour" << std::endl;
          return 0;
        }

        //Connect the texture colour to the ID and info
        TextureInfo* const textureInfoPtr = &idTextureMap[textureId];
        colourTexturePtrMap[colourArray] = textureInfoPtr;
        textureInfoPtr->colour = colourArray;
        textureInfoPtr->isColour = true;

        //Handle filtering and mipmaps
        glTextureParameteri(textureId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(textureId, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glGenerateTextureMipmap(textureId);

        return textureId;
      }

      /*
       - Load a texture from a file and return its ID
         - flipTexture controls whether the texture is flipped or not
         - srgbTexture controls whether the texture is treated as sRGB
       - Caches / deduplicates same-file textures
       - Returns 0 on failure
      */
      GLuint loadTexture(const std::string& texturePath, bool flipTexture, bool srgbTexture) {
        //Check if texture has already been loaded, with the same settings
        const unsigned char extraData = ((int)flipTexture << 0) | ((int)srgbTexture << 1);
        const std::string textureString = texturePath + std::to_string(extraData);
        if (stringTexturePtrMap.contains(textureString)) {
          TextureInfo* const textureInfo = stringTexturePtrMap[textureString];

          textureInfo->refCount++;
          return textureInfo->id;
        }

        //Handle texture flips
        if (flipTexture) {
          stbi_set_flip_vertically_on_load_thread(1);
        }

        //Read image data
        int width = 0, height = 0, nChannels = 0;
        unsigned char* const data = stbi_load(texturePath.c_str(), &width, &height, &nChannels, 0);
        if (flipTexture) {
          stbi_set_flip_vertically_on_load_thread(0);
        }

        if (data == nullptr) {
          ammonite::utils::warning << "Failed to load texture '" << texturePath << "'" \
                                   << std::endl;
          return 0;
        }

        //Decide the format of the texture and data
        GLenum textureFormat = 0, dataFormat = 0;
        if (!decideTextureFormat(nChannels, srgbTexture, &textureFormat, &dataFormat)) {
          ammonite::utils::warning << "Failed to load texture '" << texturePath << "'" \
                                   << std::endl;
          stbi_image_free(data);
          return 0;
        }

        //Create the texture and free the image data
        const unsigned int mipmapLevels = calculateMipmapLevels(width, height);
        const GLuint textureId = createTexture(width, height, data, dataFormat, textureFormat,
                                               (GLint)mipmapLevels);
        stbi_image_free(data);
        if (textureId == 0) {
          ammonite::utils::warning << "Failed to load texture '" << texturePath << "'" \
                                   << std::endl;
          return 0;
        }

        //Connect the texture string to the ID and info
        TextureInfo* const textureInfoPtr = &idTextureMap[textureId];
        stringTexturePtrMap[textureString] = textureInfoPtr;
        textureInfoPtr->string = textureString;

        //When magnifying the image, use linear filtering
        glTextureParameteri(textureId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        //When minifying the image, use a linear blend of two mipmaps
        glTextureParameteri(textureId, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        //Generate mipmaps
        glGenerateTextureMipmap(textureId);

        return textureId;
      }

      /*
       - Load 6 textures as a cubemap and return its ID
         - flipTextures controls whether the textures are flipped or not
         - srgbTextures controls whether the textures are treated as sRGB
       - Returns 0 on failure
      */
      GLuint loadCubemap(std::string texturePaths[6], bool flipTextures, bool srgbTextures) {
        GLuint textureId = 0;
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &textureId);

        //Load each face into a cubemap
        bool hasCreatedStorage = false;
        for (int i = 0; i < 6; i++) {
          if (flipTextures) {
            stbi_set_flip_vertically_on_load_thread(1);
          }

          //Read the image data
          int width = 0, height = 0, nChannels = 0;
          unsigned char* const imageData = stbi_load(texturePaths[i].c_str(), &width,
                                                     &height, &nChannels, 0);

          //Disable texture flipping, to avoid interfering with future calls
          if (flipTextures) {
            stbi_set_flip_vertically_on_load_thread(0);
          }

          //Decide the format of the texture and data
          GLenum internalFormat = 0, dataFormat = 0;
          if (!decideTextureFormat(nChannels, srgbTextures,
              &internalFormat, &dataFormat)) {
            //Free image data, destroy texture and return
            ammonite::utils::warning << "Failed to load '" << texturePaths[i] << "'" << std::endl;
            stbi_image_free(imageData);
            glDeleteTextures(1, &textureId);

            return 0;
          }

          //Only create texture storage once
          if (!hasCreatedStorage) {
            glTextureStorage2D(textureId,
              (GLint)ammonite::textures::internal::calculateMipmapLevels(width, height),
              internalFormat, width, height);
            hasCreatedStorage = true;
          }

          //Fill the texture with each face
          if (imageData != nullptr) {
            glTextureSubImage3D(textureId, 0, 0, 0, i, width, height, 1, dataFormat,
                                GL_UNSIGNED_BYTE, imageData);
            stbi_image_free(imageData);
          } else {
            //Free image data, destroy texture and return
            ammonite::utils::warning << "Failed to load '" << texturePaths[i] << "'" << std::endl;
            stbi_image_free(imageData);
            glDeleteTextures(1, &textureId);

            return 0;
          }
        }

        glTextureParameteri(textureId, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(textureId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(textureId, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(textureId, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(textureId, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glGenerateTextureMipmap(textureId);

        return textureId;
      }
    }
  }
}
