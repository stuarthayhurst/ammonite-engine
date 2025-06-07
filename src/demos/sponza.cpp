#include <iostream>
#include <string>
#include <vector>

#include <ammonite/ammonite.hpp>

#include "sponza.hpp"

namespace sponzaDemo {
  namespace {
    std::vector<AmmoniteId> loadedModelIds;
  }

  bool demoExit() {
    for (unsigned int i = 0; i < loadedModelIds.size(); i++) {
      ammonite::models::deleteModel(loadedModelIds[i]);
    }

    return true;
  }

  bool preEngineInit() {
    return true;
  }

  bool postEngineInit() {
    const AmmoniteId screenId = ammonite::splash::getActiveSplashScreenId();

    //Load models from a set of objects and textures
    const std::string models[][2] = {
      {"assets-experimental/intel-assets/main_sponza/NewSponza_Main_glTF_002.gltf", ""},
      {"assets/cube.obj", "assets/flat.png"}
    };
    const unsigned int modelCount = sizeof(models) / sizeof(models[0]);

    long unsigned int vertexCount = 0;
    for (unsigned int i = 0; i < modelCount; i++) {
      //Load model
      const AmmoniteId modelId = ammonite::models::createModel(models[i][0]);
      loadedModelIds.push_back(modelId);

      //Prevent total failure if a model fails
      if (modelId == 0) {
        ammonite::utils::warning << "Failed to load '" << models[i][0] << "'" << std::endl;
        continue;
      }

      //Sum vertices and load texture if given
      vertexCount += ammonite::models::getVertexCount(loadedModelIds[i]);
      if (!models[i][1].empty()) {
        if (!ammonite::models::applyTexture(loadedModelIds[i], AMMONITE_DIFFUSE_TEXTURE,
                                           models[i][1], true)) {
          ammonite::utils::warning << "Failed to apply texture '" << models[i][1] \
                                   << "' to '" << models[i][0] << "'" << std::endl;
        }
      }

      //Update splash screen
      ammonite::splash::setSplashScreenProgress(screenId, float(i + 1) / float(modelCount + 1));
      ammonite::renderer::drawFrame();
    }

    //Copy last loaded model
    ammonite::models::position::setPosition(loadedModelIds[modelCount - 1], glm::vec3(0.0f, 20.0f, 0.0f));
    ammonite::models::position::scaleModel(loadedModelIds[modelCount - 1], 0.25f);

    const AmmoniteId skyboxId = ammonite::skybox::loadDirectory("assets-experimental/skybox/");
    if (skyboxId != 0) {
      ammonite::skybox::setActiveSkybox(skyboxId);
    } else {
      ammonite::utils::warning << "Skybox failed to load" << std::endl;
    }

    ammonite::utils::status << "Loaded " << vertexCount << " vertices" << std::endl;

    //Update splash screen
    ammonite::splash::setSplashScreenProgress(screenId, 1.0f);
    ammonite::renderer::drawFrame();

    //Set light source properties
    const AmmoniteId lightId = ammonite::lighting::createLightSource();
    ammonite::lighting::properties::setPower(lightId, 50.0f);
    ammonite::lighting::linkModel(lightId, loadedModelIds[modelCount - 1]);
    ammonite::lighting::setAmbientLight(glm::vec3(0.1f, 0.1f, 0.1f));

    //Set the camera position
    const AmmoniteId cameraId = ammonite::camera::getActiveCamera();
    ammonite::camera::setPosition(cameraId, glm::vec3(5.0f, 1.5f, 0.0f));
    ammonite::camera::setAngle(cameraId, glm::radians(270.0f), glm::radians(10.0f));

    return true;
  }

  bool rendererMainloop() {
    ammonite::renderer::drawFrame();
    return true;
  }
}
