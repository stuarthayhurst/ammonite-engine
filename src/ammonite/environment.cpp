#include <vector>
#include <algorithm>
#include <iostream>

#include <stb/stb_image.h>
#include <GL/glew.h>

#include "internal/textures.hpp"

namespace ammonite {
  namespace environment {
    namespace {
      //Tracker for loaded skyboxes
      std::vector<int> skyboxTracker;
      int activeSkybox = 0;

      //Constants for loading assumptions
      const bool ASSUME_FLIP_FACES = false;
      const bool ASSUME_SRGB_TEXTURES = false;
    }

    namespace skybox {
      int getActiveSkybox() {
        return activeSkybox;
      }

      void setActiveSkybox(int skyboxId) {
        //Set the passed skybox to active if it exists
        if (std::find(skyboxTracker.begin(), skyboxTracker.end(), skyboxId) != skyboxTracker.end()) {
          activeSkybox = skyboxId;
        }
      }

      int createSkybox(const char* texturePaths[6], bool flipTextures, bool srgbTextures, bool* externalSuccess) {
        GLuint textureId;
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &textureId);

        //Load each face into a cubemap
        int width, height, nChannels;
        bool hasCreatedStorage = false;
        for (unsigned int i = 0; i < 6; i++) {
          if (flipTextures) {
            stbi_set_flip_vertically_on_load(true);
          }

          //Read the image data
          unsigned char* imageData = stbi_load(texturePaths[i], &width, &height, &nChannels, 0);

          //Disable texture flipping, to avoid interfering with future calls
          if (flipTextures) {
            stbi_set_flip_vertically_on_load(false);
          }

          //Decide the format of the texture and data
          GLenum internalFormat;
          GLenum dataFormat;
          if (!ammonite::textures::getTextureFormat(nChannels, srgbTextures, &internalFormat, &dataFormat)) {
            //Free image data, destroy texture, set failure and return
            std::cerr << "Failed to load '" << texturePaths[i] << "'" << std::endl;
            stbi_image_free(imageData);
            glDeleteTextures(1, &textureId);

            *externalSuccess = false;
            return 0;
          }

          //Only create texture storage once
          if (!hasCreatedStorage) {
            glTextureStorage2D(textureId, 1, internalFormat, width, height);
            hasCreatedStorage = true;
          }

          //Fill the texture with each face
          if (imageData) {
            glTextureSubImage3D(textureId, 0, 0, 0, i, width, height, 1, dataFormat, GL_UNSIGNED_BYTE, imageData);
            stbi_image_free(imageData);
          } else {
            //Free image data, destroy texture, set failure and return
            std::cerr << "Failed to load '" << texturePaths[i] << "'" << std::endl;
            stbi_image_free(imageData);
            glDeleteTextures(1, &textureId);

            *externalSuccess = false;
            return 0;
          }
        }

        glTextureParameteri(textureId, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(textureId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(textureId, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(textureId, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(textureId, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        skyboxTracker.push_back(textureId);
        return textureId;
      }

      int createSkybox(const char* texturePaths[6], bool* externalSuccess) {
        return createSkybox(texturePaths, ASSUME_FLIP_FACES, ASSUME_SRGB_TEXTURES, externalSuccess);
      }

      void deleteSkybox(int skyboxId) {
        //Check the skybox exists
        auto skyboxIt = std::find(skyboxTracker.begin(), skyboxTracker.end(), skyboxId);
        if (skyboxIt != skyboxTracker.end()) {
          //Delete from vector
          skyboxTracker.erase(skyboxIt);

          //Delete skybox
          GLuint skyboxTextureId = skyboxId;
          glDeleteTextures(1, &skyboxTextureId);

          //If the active skybox is the target to delete, unset it
          if (activeSkybox == skyboxId) {
            activeSkybox = 0;
          }
        }
      }
    }
  }
}
