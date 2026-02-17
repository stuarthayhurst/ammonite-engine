#include <algorithm>
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
          std::string textureKey;
        };

        const unsigned int maxColourKeySize = sizeof(float) * 4;
        std::map<GLuint, TextureInfo> idTextureMap;
        std::unordered_map<std::string, TextureInfo*> textureKeyInfoPtrMap;
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

        void enableFilteringMipmap(GLuint textureId) {
          //When magnifying the image, use linear filtering
          glTextureParameteri(textureId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

          //When minifying the image, use a linear blend of two mipmaps
          glTextureParameteri(textureId, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

          //Generate mipmaps
          glGenerateTextureMipmap(textureId);
        }

        void connectTextureCache(GLuint textureId, const std::string& textureKey) {
          //Set the key on the texture's entry
          TextureInfo* const textureInfoPtr = &idTextureMap[textureId];
          textureInfoPtr->textureKey = textureKey;

          //Add the texture cache entry
          textureKeyInfoPtrMap[textureKey] = textureInfoPtr;
        }

        //Return the texture ID for a key, increasing the reference counter
        GLuint acquireTextureId (const std::string& textureKey) {
          TextureInfo* const textureInfo = textureKeyInfoPtrMap[textureKey];
          textureInfo->refCount++;

          return textureInfo->id;
        }
      }

      //Calculate the cache key to use for a texture and load settings
      void calculateTextureKey(const std::string& texturePath,
                               bool flipTexture, bool srgbTexture,
                               std::string* textureKey) {
        //Prepare the key's storage
        const unsigned int colourLength = maxColourKeySize;
        const unsigned int texturePathLength = texturePath.size();
        const unsigned int keyLength = colourLength + texturePathLength + 1;
        textureKey->resize(keyLength);

        //Set the colour component with zeros to avoid collisions
        std::memset(textureKey->data(), 0, colourLength);

        //Set the string and load data components
        std::memcpy(textureKey->data() + colourLength, texturePath.data(), texturePathLength);
        const unsigned char extraData = ((int)flipTexture << 0) | ((int)srgbTexture << 1);
        *(textureKey->data() + colourLength + texturePathLength) = std::to_string(extraData)[0];
      }

      namespace {
        template <unsigned int components>
        void calculateTextureKeyTemplate(const ammonite::Vec<float, components>& colour,
                                         std::string* stringPtr) {
          //Only set the colour component
          const unsigned int colourLength = sizeof(float) * components;
          stringPtr->resize(colourLength);
          std::memcpy(stringPtr->data(), &colour[0], colourLength);
        }
      }

      //Calculate the cache key to use for a 3-component colour
      void calculateTextureKey(const ammonite::Vec<float, 3>& colour,
                               std::string* textureKey) {
        calculateTextureKeyTemplate(colour, textureKey);
      }

      //Calculate the cache key to use for a 4-component colour
      void calculateTextureKey(const ammonite::Vec<float, 4>& colour,
                               std::string* textureKey) {
        calculateTextureKeyTemplate(colour, textureKey);
      }

      //Check if a cache key has been registered
      bool checkTextureKey(const std::string& textureKey) {
        return textureKeyInfoPtrMap.contains(textureKey);
      }

      /*
       - Return the ID for a reserved texture key
       - Increases the reference counter
       - Internally exposed, safer wrapper for acquireTextureId()
      */
      GLuint acquireTextureKeyId(const std::string& textureKey) {
        if (!textureKeyInfoPtrMap.contains(textureKey)) {
          ammonite::utils::warning << "Requested ID for unreserved texture" << std::endl;
          return 0;
        }

        return acquireTextureId(textureKey);
      }

      /*
       - Reserve a cache key for future use
       - Generally, the workflow for this is:
         - calculateTextureKey() -> reserveTextureKey() ->
           prepareTextureData() -> uploadTextureData()
         - prepareTextureData() is thread-safe, allowing parallelised texture loads
       - Returns the texture ID on success, 0 on failure
      */
      GLuint reserveTextureKey(const std::string& textureKey) {
        //Check the cache for the texture
        if (textureKeyInfoPtrMap.contains(textureKey)) {
          const GLuint textureId = textureKeyInfoPtrMap[textureKey]->id;
          ammonite::utils::warning << "Attempted to reserve an existing texture (ID " \
                                   << textureId << ")" << std::endl;
          return 0;
        }

        //Create a texture
        GLuint textureId = 0;
        glCreateTextures(GL_TEXTURE_2D, 1, &textureId);
        if (textureId == 0) {
          ammonite::utils::warning << "Failed to create texture" << std::endl;
          return 0;
        }

        //Add the texture to the tracker
        if (idTextureMap.contains(textureId)) {
          ammonite::utils::warning << "Texture ID (" << textureId \
                                   << ") already exists, not reserving texture" << std::endl;
          return 0;
        }
        idTextureMap[textureId] = {.id = textureId, .refCount = 1, .textureKey = ""};

        //Connect the texture string to the ID and info
        connectTextureCache(textureId, textureKey);

        return textureId;
      }

      /*
       - Upload data for the texture of a reserved key
         - Data should come from prepareTextureData();
         - This must be called before textureId can be rendered from
       - The TextureData structure can't be reused after this
       - Returns true on success, 0 on failure
      */
      bool uploadTextureData(GLuint textureId, const TextureData& textureData) {

        //Check texture size is within limits
        GLint maxTextureSize = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
        if (textureData.width > maxTextureSize || textureData.height > maxTextureSize) {
          ammonite::utils::warning << "Attempted to create a texture of unsupported size (" \
                                   << textureData.width << " x " << textureData.height \
                                   << ")" << std::endl;
          return false;
        }

        //Decide the format of the texture and data
        GLenum textureFormat = 0, dataFormat = 0;
        if (!decideTextureFormat(textureData.numChannels, textureData.srgbTexture,
                                 &textureFormat, &dataFormat)) {
          ammonite::utils::warning << "Failed to upload texture (ID " \
                                   << textureId << ")" << std::endl;
          stbi_image_free(textureData.data);
          return false;
        }

        //Create and fill texture storage
        const unsigned int textureLevels = calculateMipmapLevels(textureData.width,
                                                                 textureData.height);
        glTextureStorage2D(textureId, (GLint)textureLevels, textureFormat,
                           textureData.width, textureData.height);
        glTextureSubImage2D(textureId, 0, 0, 0, textureData.width, textureData.height,
                            dataFormat, GL_UNSIGNED_BYTE, textureData.data);

        //Free the texture data's storage
        stbi_image_free(textureData.data);

        //Handle filtering and mipmaps
        enableFilteringMipmap(textureId);

        return true;
      }


      /*
       - Load texture data for future upload
         - flipTextures controls whether the textures are flipped or not
         - srgbTextures controls whether the textures are treated as sRGB
       - Guaranteed to be thread-safe
       - Returns true on success, false on failure
      */
      bool prepareTextureData(const std::string& texturePath, bool flipTexture,
                              bool srgbTexture, TextureData* textureData) {

        //Handle texture flips
        if (flipTexture) {
          stbi_set_flip_vertically_on_load_thread(1);
        }

        //Read image data
        textureData->data = stbi_load(texturePath.c_str(), &textureData->width,
                                      &textureData->height, &textureData->numChannels, 0);
        if (flipTexture) {
          stbi_set_flip_vertically_on_load_thread(0);
        }

        if (textureData->data == nullptr) {
          ammonite::utils::warning << "Failed to load texture '" << texturePath \
                                   << "'" << std::endl;
          return false;
        }

        textureData->srgbTexture = srgbTexture;

        return true;
      }

      //Calculate the number of mipmaps levels to use
      unsigned int calculateMipmapLevels(int width, int height) {
        return (unsigned int)std::log2(std::max(width, height)) + 1;
      }

      //Deletes a texture created with createTexture() or load*Texture()
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
          //Remove the cache entry
          if (!textureInfoPtr->textureKey.empty()) {
            textureKeyInfoPtrMap.erase(textureInfoPtr->textureKey);
          }

          //Delete the texture
          glDeleteTextures(1, &textureInfoPtr->id);

          //Delete the tracker entry
          if (!textureInfoPtr->textureKey.empty()) {
            if (textureInfoPtr->textureKey.size() <= maxColourKeySize) {
              ammoniteInternalDebug << "Deleted storage for colour texture (ID " \
                                    << textureId << ")" << std::endl;
            } else {
              ammoniteInternalDebug << "Deleted storage for file texture (ID " \
                                    << textureId << ", '" \
                                    << textureInfoPtr->textureKey.substr(maxColourKeySize) \
                                    << "')" << std::endl;
            }
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
      GLuint createTexture(int width, int height, unsigned char* data,
                           GLenum dataFormat, GLenum textureFormat, GLint textureLevels) {
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
        glTextureSubImage2D(textureId, 0, 0, 0, width, height, dataFormat,
                            GL_UNSIGNED_BYTE, data);

        //Add the texture to the tracker
        if (idTextureMap.contains(textureId)) {
          ammonite::utils::warning << "Texture ID (" << textureId \
                                   << ") already exists, not creating texture" << std::endl;
          return 0;
        }
        idTextureMap[textureId] = {.id = textureId, .refCount = 1, .textureKey = ""};

        return textureId;
      }

      /*
       - Load a texture from a colour and return its ID
       - Caches / deduplicates identical solid colour textures
       - Returns 0 on failure
      */
      namespace {
        template <unsigned int components> requires (components == 3 || components == 4)
        GLuint loadSolidTextureTemplate(const ammonite::Vec<float, components>& colour) {
          //Calculate the texture's cache key
          std::string textureKey;
          calculateTextureKey(colour, &textureKey);

          //Check the cache for the texture
          if (textureKeyInfoPtrMap.contains(textureKey)) {
            return acquireTextureId(textureKey);
          }

          //Decide the format of the texture and data
          GLenum textureFormat = 0, dataFormat = 0;
          if (!decideTextureFormat(components, false, &textureFormat, &dataFormat)) {
            ammonite::utils::warning << "Failed to load texture from colour" << std::endl;
            return 0;
          }

          //Convert the colour into the required format
          ammonite::Vec<float, components> scaledColour = {0};
          ammonite::scale(colour, 255.0f, scaledColour);
          unsigned char data[components] = {
            (unsigned char)scaledColour[0],
            (unsigned char)scaledColour[1],
            (unsigned char)scaledColour[2]
          };
          if constexpr (components == 4) {
            data[3] = (unsigned char)scaledColour[3];
          }

          //Create the texture and free the image data
          const unsigned int mipmapLevels = calculateMipmapLevels(1, 1);
          const GLuint textureId = createTexture(1, 1, data, dataFormat, textureFormat,
                                                 (GLint)mipmapLevels);
          if (textureId == 0) {
            ammonite::utils::warning << "Failed to load texture from colour" << std::endl;
            return 0;
          }

          //Connect the texture string to the ID and info
          connectTextureCache(textureId, textureKey);

          //Handle filtering and mipmaps
          enableFilteringMipmap(textureId);

          return textureId;
        }
      }

      GLuint loadSolidTexture(const ammonite::Vec<float, 3>& colour) {
        return loadSolidTextureTemplate(colour);
      }

      GLuint loadSolidTexture(const ammonite::Vec<float, 4>& colour) {
        return loadSolidTextureTemplate(colour);
      }

      /*
       - Load a texture from a file and return its ID
         - flipTexture controls whether the texture is flipped or not
         - srgbTexture controls whether the texture is treated as sRGB
       - Caches / deduplicates same-file textures
       - Returns 0 on failure
      */
      GLuint loadTexture(const std::string& texturePath, bool flipTexture,
                         bool srgbTexture) {
        //Calculate the texture's cache key
        std::string textureKey;
        calculateTextureKey(texturePath, flipTexture, srgbTexture, &textureKey);

        //Use texture cache, if already loaded / reserved
        if (checkTextureKey(textureKey)) {
          return acquireTextureId(textureKey);
        }

        //Reserve the texture key before loading
        const GLuint textureId = textures::internal::reserveTextureKey(textureKey);
        if (textureId == 0) {
          ammonite::utils::warning << "Failed to reserve texture ID" << std::endl;
          return 0;
        }

        //Load the texture data
        TextureData textureData;
        if (!textures::internal::prepareTextureData(texturePath, flipTexture,
                                                    srgbTexture, &textureData)) {
          deleteTexture(textureId);
          return 0;
        }

        //Upload the texture data
        if (!textures::internal::uploadTextureData(textureId, textureData)) {
          deleteTexture(textureId);
          return 0;
        }

        return textureId;
      }

      /*
       - Load 6 textures as a cubemap and return its ID
         - flipTextures controls whether the textures are flipped or not
         - srgbTextures controls whether the textures are treated as sRGB
       - Returns 0 on failure
      */
      GLuint loadCubemap(std::string texturePaths[6], bool flipTextures,
                         bool srgbTextures) {
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
            ammonite::utils::warning << "Failed to load '" << texturePaths[i] \
                                     << "'" << std::endl;
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
            ammonite::utils::warning << "Failed to load '" << texturePaths[i] \
                                     << "'" << std::endl;
            stbi_image_free(imageData);
            glDeleteTextures(1, &textureId);

            return 0;
          }
        }

        //Handle filtering and mipmaps
        glTextureParameteri(textureId, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(textureId, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(textureId, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        enableFilteringMipmap(textureId);

        return textureId;
      }
    }
  }
}
