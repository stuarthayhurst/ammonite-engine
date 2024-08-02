#include <algorithm>
#include <filesystem>
#include <iostream>
#include <unordered_set>
#include <vector>

#include <stb/stb_image.h>
#include <GL/glew.h>

#include "graphics/internal/internalTextures.hpp"
#include "utils/logging.hpp"

//Loading assumptions
#define ASSUME_FLIP_FACES false
#define ASSUME_SRGB_TEXTURES false

namespace ammonite {
  namespace environment {
    namespace {
      //Tracker for loaded skyboxes
      std::unordered_set<int> skyboxTracker;
      int activeSkybox = -1;
    }

    namespace skybox {
      int getActiveSkybox() {
        return activeSkybox;
      }

      void setActiveSkybox(int skyboxId) {
        //Set the passed skybox to active if it exists
        if (skyboxTracker.contains(skyboxId)) {
          activeSkybox = skyboxId;
        }
      }

      int createSkybox(const char* texturePaths[6], bool flipTextures, bool srgbTextures,
                       bool* externalSuccess) {
        GLuint textureId;
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &textureId);

        //Load each face into a cubemap
        int width, height, nChannels;
        bool hasCreatedStorage = false;
        for (unsigned int i = 0; i < 6; i++) {
          if (flipTextures) {
            stbi_set_flip_vertically_on_load_thread(true);
          }

          //Read the image data
          unsigned char* imageData = stbi_load(texturePaths[i], &width, &height, &nChannels, 0);

          //Disable texture flipping, to avoid interfering with future calls
          if (flipTextures) {
            stbi_set_flip_vertically_on_load_thread(false);
          }

          //Decide the format of the texture and data
          GLenum internalFormat;
          GLenum dataFormat;
          if (!ammonite::textures::internal::getTextureFormat(nChannels, srgbTextures,
              &internalFormat, &dataFormat)) {
            //Free image data, destroy texture, set failure and return
            ammonite::utils::warning << "Failed to load '" << texturePaths[i] << "'" << std::endl;
            stbi_image_free(imageData);
            glDeleteTextures(1, &textureId);

            *externalSuccess = false;
            return -1;
          }

          //Only create texture storage once
          if (!hasCreatedStorage) {
            glTextureStorage2D(textureId,
                               ammonite::textures::internal::calculateMipmapLevels(width, height),
                               internalFormat, width, height);
            hasCreatedStorage = true;
          }

          //Fill the texture with each face
          if (imageData) {
            glTextureSubImage3D(textureId, 0, 0, 0, i, width, height, 1, dataFormat,
                                GL_UNSIGNED_BYTE, imageData);
            stbi_image_free(imageData);
          } else {
            //Free image data, destroy texture, set failure and return
            ammonite::utils::warning << "Failed to load '" << texturePaths[i] << "'" << std::endl;
            stbi_image_free(imageData);
            glDeleteTextures(1, &textureId);

            *externalSuccess = false;
            return -1;
          }
        }

        glTextureParameteri(textureId, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(textureId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(textureId, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(textureId, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(textureId, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glGenerateTextureMipmap(textureId);

        skyboxTracker.insert(textureId);
        return textureId;
      }

      int createSkybox(const char* texturePaths[6], bool* externalSuccess) {
        return createSkybox(texturePaths, ASSUME_FLIP_FACES, ASSUME_SRGB_TEXTURES,
                            externalSuccess);
      }

      int loadDirectory(const char* directoryPath, bool flipTextures,  bool srgbTextures,
                        bool* externalSuccess) {
        //Create filesystem directory iterator
        std::filesystem::directory_iterator it;
        try {
          const std::filesystem::path skyboxDir{directoryPath};
          it = std::filesystem::directory_iterator{skyboxDir};
        } catch (const std::filesystem::filesystem_error&) {
            *externalSuccess = false;
            ammonite::utils::warning << "Failed to scan '" << directoryPath << "'" << std::endl;
            return -1;
        }

        //Find files to send to next stage
        std::vector<std::string> faces(0);
        for (auto const& fileName : it) {
          std::filesystem::path filePath{fileName};
          faces.push_back(std::string(filePath));
        }

        //Check we have at least 6 faces
        if (faces.size() < 6) {
          *externalSuccess = false;
          ammonite::utils::warning << "Failed to load '" << directoryPath \
                                   << "', needs at least 6 faces" << std::endl;
          return -1;
        }

        //Select 6 faces using their names
        const char* skyboxFaces[6];
        std::string faceOrder[6] = {"right", "left", "top", "bottom", "front", "back"};
        int targetFace = 0;
        while (targetFace < 6) {
          //Look for the target face
          int foundIndex = -1;
          for (unsigned int i = 0; i < faces.size(); i++) {
            //Check if the current face is the desired face
            if (faces[i].contains(faceOrder[targetFace])) {
              foundIndex = i;
              break;
            }
          }

          //Either add the face and repeat / break, or give up
          if (foundIndex != -1) {
            skyboxFaces[targetFace] = faces[foundIndex].c_str();
            targetFace++;
          } else {
            *externalSuccess = false;
            ammonite::utils::warning << "Failed to load '" << directoryPath << "'" << std::endl;
            return -1;
          }
        }

        //Hand off to regular skybox creation
        return createSkybox(skyboxFaces, flipTextures, srgbTextures, externalSuccess);
      }

      int loadDirectory(const char* directoryPath, bool* externalSuccess) {
        return loadDirectory(directoryPath, ASSUME_FLIP_FACES,
                             ASSUME_SRGB_TEXTURES, externalSuccess);
      }

      void deleteSkybox(int skyboxId) {
        //Check the skybox exists and remove
        if (skyboxTracker.contains(skyboxId)) {
          //Delete from set
          skyboxTracker.erase(skyboxId);

          //Delete skybox
          GLuint skyboxTextureId = skyboxId;
          glDeleteTextures(1, &skyboxTextureId);

          //If the active skybox is the target to delete, unset it
          if (activeSkybox == skyboxId) {
            activeSkybox = -1;
          }
        }
      }
    }
  }
}
