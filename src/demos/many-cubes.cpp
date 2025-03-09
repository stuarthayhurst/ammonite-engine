#include <cmath>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include <ammonite/ammonite.hpp>
#include <glm/glm.hpp>

#include "many-cubes.hpp"

namespace manyCubesDemo {
  namespace {
    std::vector<AmmoniteId> loadedModelIds;
    unsigned int sideLength = 0;

    const int unsigned lightCount = 50;
    AmmoniteId lightSourceIds[lightCount];
    glm::vec3 lightSourcePositions[lightCount];
  }

  bool demoExit() {
    for (unsigned int i = 0; i < loadedModelIds.size(); i++) {
      ammonite::models::deleteModel(loadedModelIds[i]);
    }

    return true;
  }

  bool preRendererInit() {
    return true;
  }

  bool postRendererInit() {
    AmmoniteId screenId = ammonite::interface::getActiveLoadingScreenId();

    //Load models from a set of objects and textures
    std::string models[2] = {"assets/cube.obj", "assets/flat.png"};
    unsigned int modelCount = 10000;

    //Load cube model
    AmmoniteId modelId = ammonite::models::createModel(models[0]);
    loadedModelIds.push_back(modelId);
    bool applied = ammonite::models::applyTexture(loadedModelIds[0], AMMONITE_DIFFUSE_TEXTURE,
                                                models[1], true);
    long unsigned int vertexCount = ammonite::models::getVertexCount(loadedModelIds[0]);
    if (modelId == 0 || !applied) {
      demoExit();
      return false;
    }

    for (unsigned int i = 1; i < modelCount; i++) {
      //Load model and count vertices
      loadedModelIds.push_back(ammonite::models::copyModel(loadedModelIds[0]));
      vertexCount += ammonite::models::getVertexCount(loadedModelIds[i]);

      //Update loading screen
      float progress = float(i + 1) / float(modelCount + 1);
      ammonite::interface::setLoadingScreenProgress(screenId, progress);
      ammonite::renderer::drawFrame();
    }

    ammonite::utils::status << "Loaded " << vertexCount << " vertices" << std::endl;

    //Update loading screen
    ammonite::interface::setLoadingScreenProgress(screenId, 1.0f);
    ammonite::renderer::drawFrame();

    //Reposition cubes
    sideLength = (unsigned int)std::sqrt(modelCount);
    for (unsigned int x = 0; x < sideLength; x++) {
      for (unsigned int y = 0; y < sideLength; y++) {
        ammonite::models::position::setPosition(loadedModelIds[((std::size_t)x * sideLength) + y],
          glm::vec3(2.0f * float(x), 0.0f, 2.0f * float(y)));
      }
    }

    //Create light sources
    ammonite::lighting::setAmbientLight(glm::vec3(0.1f, 0.1f, 0.1f));
    for (unsigned int i = 0; i < lightCount; i++) {
      lightSourceIds[i] = ammonite::lighting::createLightSource();
      lightSourcePositions[i] = glm::vec3(ammonite::utils::randomInt(0, sideLength),
                                          4.0f,
                                          ammonite::utils::randomInt(0, sideLength));
      float red = (float)ammonite::utils::randomDouble(1.0);
      float green = (float)ammonite::utils::randomDouble(1.0);
      float blue = (float)ammonite::utils::randomDouble(1.0);
      ammonite::lighting::properties::setPower(lightSourceIds[i], 50.0f);
      ammonite::lighting::properties::setColour(lightSourceIds[i], glm::vec3(red, green, blue));
    }

    //Set the camera position
    AmmoniteId cameraId = ammonite::camera::getActiveCamera();
    ammonite::camera::setPosition(cameraId, glm::vec3(0.0f, 12.0f, 0.0f));
    ammonite::camera::setHorizontal(cameraId, glm::radians(45.0f));
    ammonite::camera::setVertical(cameraId, glm::radians(-20.0f));

    return true;
  }

  bool rendererMainloop() {
    //Update light source positions
    for (unsigned int i = 0; i < lightCount; i++) {
      bool invalid = true;
      AmmoniteId currLightSourceId = lightSourceIds[i];
      float x = 0.0f, z = 0.0f;
      while (invalid) {
        x = (float)ammonite::utils::randomDouble(-1.0, 1.0);
        z = (float)ammonite::utils::randomDouble(-1.0, 1.0);

        x += lightSourcePositions[i].x;
        z += lightSourcePositions[i].z;
        if (x >= 0.0f && x <= (float)sideLength) {
          if (z >= 0.0f && z <= (float)sideLength) {
            invalid = false;
          }
        }
      }

      lightSourcePositions[i] = glm::vec3(x, 4.0f, z);
      ammonite::lighting::properties::setGeometry(currLightSourceId, lightSourcePositions[i]);
    }

    ammonite::renderer::drawFrame();
    return true;
  }
}
