#include <vector>
#include <string>
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

      int createSkybox(std::vector<std::string> texturePaths, bool srgbTextures, bool* externalSuccess) {
        if (texturePaths.size() != 6) {
          std::cerr << "Skyboxes require 6 texture paths" << std::endl;
          *externalSuccess = false;
          return 0;
        }

        GLuint textureId;
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &textureId);

        //Load each face into a cubemap
        int width, height, nChannels;
        bool createdStorage = false;
        for (unsigned int i = 0; i < 6; i++) {
          //Read the image data
          unsigned char* imageData = stbi_load(texturePaths[i].c_str(), &width, &height, &nChannels, 0);

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
          if (!createdStorage) {
            glTextureStorage2D(textureId, 1, internalFormat, width, height);
            createdStorage = true;
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

      int createSkybox(std::vector<std::string> texturePaths, bool* externalSuccess) {
        return createSkybox(texturePaths, false, externalSuccess);
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
