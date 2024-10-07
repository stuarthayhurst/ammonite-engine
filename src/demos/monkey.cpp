#include <iostream>
#include <string>
#include <vector>

#include "../ammonite/ammonite.hpp"

namespace monkeyDemo {
  namespace {
    std::vector<AmmoniteId> loadedModelIds;
  }

  int demoExit() {
    for (unsigned int i = 0; i < loadedModelIds.size(); i++) {
      ammonite::models::deleteModel(loadedModelIds[i]);
    }

    return 0;
  }

  int preRendererInit() {
    return 0;
  }

  int postRendererInit() {
    AmmoniteId screenId = ammonite::interface::getActiveLoadingScreen();

    //Load models from a set of objects and textures
    std::string models[][2] = {
      {"assets/suzanne.obj", "assets/gradient.png"},
      {"assets/cube.obj", "assets/flat.png"}
    };
    unsigned int modelCount = sizeof(models) / sizeof(models[0]);

    bool success = true;
    long unsigned int vertexCount = 0;
    for (unsigned int i = 0; i < modelCount; i++) {
      //Load model
      loadedModelIds.push_back(ammonite::models::createModel(models[i][0], &success));

      if (!success) {
        //Prevent total failure if models fail
        success = true;
        ammonite::utils::warning << "Failed to load " << models[i][0] << std::endl;
        continue;
      }

      //Sum vertices and load texture if given
      vertexCount += ammonite::models::getVertexCount(loadedModelIds[i]);
      if (models[i][1] != "") {
        ammonite::models::applyTexture(loadedModelIds[i], AMMONITE_DIFFUSE_TEXTURE, models[i][1], true, &success);
      }

      //Update loading screen
      ammonite::interface::setLoadingScreenProgress(screenId, float(i + 1) / float(modelCount + 1));
      ammonite::renderer::drawFrame();
    }

    if (!success) {
      demoExit();
      return -1;
    }

    //Copy last loaded model
    loadedModelIds.push_back(ammonite::models::copyModel(loadedModelIds[modelCount - 1]));
    vertexCount += ammonite::models::getVertexCount(loadedModelIds[modelCount]);
    ammonite::models::position::setPosition(loadedModelIds[modelCount], glm::vec3(4.0f, 4.0f, 4.0f));
    ammonite::models::position::scaleModel(loadedModelIds[modelCount], 0.25f);
    modelCount++;

    ammonite::utils::status << "Loaded " << vertexCount << " vertices" << std::endl;

    //Update loading screen
    ammonite::interface::setLoadingScreenProgress(screenId, 1.0f);
    ammonite::renderer::drawFrame();

    //Example translation, scale and rotation
    ammonite::models::position::translateModel(loadedModelIds[0], glm::vec3(-2.0f, 0.0f, 0.0f));
    ammonite::models::position::scaleModel(loadedModelIds[0], 0.8f);
    ammonite::models::position::rotateModel(loadedModelIds[0], glm::vec3(0.0f, 0.0f, 0.0f));

    //Set light source properties
    AmmoniteId lightId = ammonite::lighting::createLightSource();
    ammonite::lighting::properties::setPower(lightId, 50.0f);
    ammonite::lighting::linkModel(lightId, loadedModelIds[modelCount - 1]);
    ammonite::lighting::setAmbientLight(glm::vec3(0.1f, 0.1f, 0.1f));

    //Set the camera position
    AmmoniteId cameraId = ammonite::camera::getActiveCamera();
    ammonite::camera::setPosition(cameraId, glm::vec3(0.0f, 0.0f, 5.0f));
    ammonite::camera::setHorizontal(cameraId, glm::radians(180.0f));
    ammonite::camera::setVertical(cameraId, glm::radians(0.0f));

    return 0;
  }

  int rendererMainloop() {
    ammonite::renderer::drawFrame();
    return 0;
  }
}
