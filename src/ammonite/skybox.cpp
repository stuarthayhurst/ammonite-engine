#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

extern "C" {
  #include <epoxy/gl.h>
  #include <stb/stb_image.h>
}

#include "skybox.hpp"

#include "enums.hpp"
#include "graphics/textures.hpp"
#include "utils/debug.hpp"
#include "utils/id.hpp"
#include "utils/logging.hpp"

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
      GLuint textureId = textures::internal::loadCubemap(texturePaths, flipTextures, srgbTextures);
      if (textureId == 0) {
        ammonite::utils::warning << "Failed to create skybox" << std::endl;
        return 0;
      }

      skyboxTracker.insert(textureId);
      return textureId;
    }

    /*
     - Load 6 textures as a skybox and return its ID
     - Returns 0 on failure
    */
    AmmoniteId createSkybox(std::string texturePaths[6]) {
      return createSkybox(texturePaths, ASSUME_FLIP_SKYBOX_FACES, ASSUME_SRGB_TEXTURES);
    }

    /*
     - Load 6 textures from a directory as a skybox and return its ID
       - flipTextures controls whether the textures are flipped or not
       - srgbTextures controls whether the textures are treated as sRGB
     - Returns 0 on failure
    */
    AmmoniteId loadDirectory(const std::string& directoryPath, bool flipTextures, bool srgbTextures) {
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
        const std::filesystem::path& filePath{fileName};
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
      const std::string faceOrder[6] = {"right", "left", "top", "bottom", "front", "back"};
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
    AmmoniteId loadDirectory(const std::string& directoryPath) {
      return loadDirectory(directoryPath, ASSUME_FLIP_SKYBOX_FACES,
                           ASSUME_SRGB_TEXTURES);
    }

    void deleteSkybox(AmmoniteId skyboxId) {
      //Check the skybox exists and remove
      if (skyboxTracker.contains(skyboxId)) {
        //Delete from set
        skyboxTracker.erase(skyboxId);
        ammoniteInternalDebug << "Deleted storage for skybox (ID " << skyboxId \
                              << ")" << std::endl;

        //Delete skybox
        const GLuint skyboxTextureId = skyboxId;
        glDeleteTextures(1, &skyboxTextureId);

        //If the active skybox is the target to delete, unset it
        if (activeSkybox == skyboxId) {
          activeSkybox = 0;
        }
      }
    }
  }
}
