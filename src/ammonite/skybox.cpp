#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include <stb/stb_image.h>
#include <GL/glew.h>

#include "graphics/internal/internalTextures.hpp"
#include "utils/logging.hpp"
#include "types.hpp"

//Loading assumptions
#define ASSUME_FLIP_FACES false
#define ASSUME_SRGB_TEXTURES false

namespace ammonite {
  namespace {
    //Tracker for loaded skyboxes
    std::unordered_set<AmmoniteId> skyboxTracker;
    AmmoniteId activeSkybox = 0;
  }

  namespace skybox {
    AmmoniteId getActiveSkybox() {
      return activeSkybox;
    }

    void setActiveSkybox(AmmoniteId skyboxId) {
      //Set the passed skybox to active if it exists
      if (skyboxTracker.contains(skyboxId)) {
        activeSkybox = skyboxId;
      }
    }

    /*
     - Load 6 textures as a skybox and return its ID
       - flipTextures controls whether the textures are flipped or not
       - srgbTextures controls whether the textures are treated as sRGB
     - Returns 0 on failure
    */
    AmmoniteId createSkybox(std::string texturePaths[6], bool flipTextures, bool srgbTextures) {
      GLuint textureId;
      glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &textureId);

      //Load each face into a cubemap
      int width, height, nChannels;
      bool hasCreatedStorage = false;
      for (int i = 0; i < 6; i++) {
        if (flipTextures) {
          stbi_set_flip_vertically_on_load_thread(true);
        }

        //Read the image data
        unsigned char* imageData = stbi_load(texturePaths[i].c_str(), &width, &height, &nChannels, 0);

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
        if (imageData) {
          glTextureSubImage3D(textureId, 0, 0, 0, i, width, height, 1, dataFormat,
                              GL_UNSIGNED_BYTE, imageData);
          stbi_image_free(imageData);
        } else {
          //Free image data, destroy texture, set failure and return
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

      skyboxTracker.insert(textureId);
      return textureId;
    }

    /*
     - Load 6 textures as a skybox and return its ID
     - Returns 0 on failure
    */
    AmmoniteId createSkybox(std::string texturePaths[6]) {
      return createSkybox(texturePaths, ASSUME_FLIP_FACES, ASSUME_SRGB_TEXTURES);
    }

    /*
     - Load 6 textures from a directory as a skybox and return its ID
       - flipTextures controls whether the textures are flipped or not
       - srgbTextures controls whether the textures are treated as sRGB
     - Returns 0 on failure
    */
    AmmoniteId loadDirectory(std::string directoryPath, bool flipTextures, bool srgbTextures) {
      //Create filesystem directory iterator
      std::filesystem::directory_iterator it;
      try {
        const std::filesystem::path skyboxDir{directoryPath};
        it = std::filesystem::directory_iterator{skyboxDir};
      } catch (const std::filesystem::filesystem_error&) {
        ammonite::utils::warning << "Failed to scan '" << directoryPath << "'" << std::endl;
        return 0;
      }

      //Find files to send to next stage
      std::vector<std::string> faces(0);
      for (auto const& fileName : it) {
        std::filesystem::path filePath{fileName};
        faces.push_back(std::string(filePath));
      }

      //Check we have at least 6 faces
      if (faces.size() < 6) {
        ammonite::utils::warning << "Failed to load '" << directoryPath \
                                 << "', needs at least 6 faces" << std::endl;
        return 0;
      }

      //Select 6 faces using their names
      std::string skyboxFaces[6];
      std::string faceOrder[6] = {"right", "left", "top", "bottom", "front", "back"};
      int targetFace = 0;
      while (targetFace < 6) {
        //Look for the target face
        int foundIndex = -1;
        for (unsigned int i = 0; i < faces.size(); i++) {
          //Check if the current face is the desired face
          if (faces[i].contains(faceOrder[targetFace])) {
            foundIndex = (int)i;
            break;
          }
        }

        //Either add the face and repeat / break, or give up
        if (foundIndex != -1) {
          skyboxFaces[targetFace] = faces[foundIndex];
          targetFace++;
        } else {
          ammonite::utils::warning << "Failed to load '" << directoryPath << "'" << std::endl;
          return 0;
        }
      }

      //Hand off to regular skybox creation
      return createSkybox(skyboxFaces, flipTextures, srgbTextures);
    }

    /*
     - Load 6 textures from a directory as a skybox and return its ID
     - Returns 0 on failure
    */
    AmmoniteId loadDirectory(std::string directoryPath) {
      return loadDirectory(directoryPath, ASSUME_FLIP_FACES,
                           ASSUME_SRGB_TEXTURES);
    }

    void deleteSkybox(AmmoniteId skyboxId) {
      //Check the skybox exists and remove
      if (skyboxTracker.contains(skyboxId)) {
        //Delete from set
        skyboxTracker.erase(skyboxId);

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
