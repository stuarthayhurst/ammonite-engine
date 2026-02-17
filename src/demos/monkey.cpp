#include <iostream>
#include <string>
#include <vector>

#include <ammonite/ammonite.hpp>

#include "monkey.hpp"

namespace monkeyDemo {
  namespace {
    std::vector<AmmoniteId> loadedModelIds;

    const ammonite::Vec<float, 3> cameraPosition = {0.0f, 0.0f, 5.0f};
    const ammonite::Vec<float, 3> ambientLight = {0.1f, 0.1f, 0.1f};
    const ammonite::Vec<float, 3> monkeyPosition = {-2.0f, 0.0f, 0.0f};
    const ammonite::Vec<float, 3> lightPosition = {4.0f, 4.0f, 4.0f};
    const float monkeyScale = 0.8f;
    const float lightScale = 0.25f;
    const ammonite::Vec<float, 3> monkeyRotation = {0.0f, 0.0f, 0.0f};
  }

  bool demoExit() {
    for (const AmmoniteId& modelId : loadedModelIds) {
      ammonite::models::deleteModel(modelId);
    }

    return true;
  }

  bool preEngineInit() {
    return true;
  }

  bool postEngineInit() {
    const AmmoniteId screenId = ammonite::splash::getActiveSplashScreenId();

    //Load models from a set of objects and textures
    const std::string modelPaths[2] = {
      "assets/suzanne.obj", "assets/cube.obj"
    };
    const ammonite::models::AmmoniteMaterial materials[2] = {
      ammonite::models::createMaterial("assets/gradient.png", {0.5f, 0.5f, 0.5f}),
      ammonite::models::createMaterial("assets/flat.png", {0.5f, 0.5f, 0.5f})
    };
    unsigned int modelCount = sizeof(modelPaths) / sizeof(modelPaths[0]);

    long unsigned int vertexCount = 0;
    for (unsigned int i = 0; i < modelCount; i++) {
      //Load model
      const AmmoniteId modelId = ammonite::models::createModel(modelPaths[i]);
      loadedModelIds.push_back(modelId);

      //Prevent total failure if a model fails
      if (modelId == 0) {
        ammonite::utils::warning << "Failed to load '" << modelPaths[i] << "'" << std::endl;
        continue;
      }

      //Sum vertices and load texture if given
      vertexCount += ammonite::models::getVertexCount(loadedModelIds[i]);
      if (!materials[i].diffuse.textureInfo.texturePath->empty()) {
        if (!ammonite::models::applyMaterial(loadedModelIds[i], materials[i])) {
          ammonite::utils::warning << "Failed to apply texture '" \
                                   << *materials[i].diffuse.textureInfo.texturePath \
                                   << "' to '" << modelPaths[i] << "'" << std::endl;
        }
      }

      //Update splash screen
      ammonite::splash::setSplashScreenProgress(screenId, float(i + 1) / float(modelCount + 1));
      ammonite::renderer::drawFrame();
    }

    //Delete the materials
    ammonite::models::deleteMaterial(materials[0]);
    ammonite::models::deleteMaterial(materials[1]);

    //Copy last loaded model
    loadedModelIds.push_back(ammonite::models::copyModel(loadedModelIds[modelCount - 1], false));
    vertexCount += ammonite::models::getVertexCount(loadedModelIds[modelCount]);
    ammonite::models::position::setPosition(loadedModelIds[modelCount], lightPosition);
    ammonite::models::position::scaleModel(loadedModelIds[modelCount], lightScale);
    modelCount++;

    ammonite::utils::status << "Loaded " << vertexCount << " vertices" << std::endl;

    //Update splash screen
    ammonite::splash::setSplashScreenProgress(screenId, 1.0f);
    ammonite::renderer::drawFrame();

    //Example translation, scale and rotation
    ammonite::models::position::translateModel(loadedModelIds[0], monkeyPosition);
    ammonite::models::position::scaleModel(loadedModelIds[0], monkeyScale);
    ammonite::models::position::rotateModel(loadedModelIds[0], monkeyRotation);

    //Set light source properties
    const AmmoniteId lightId = ammonite::lighting::createLightSource();
    ammonite::lighting::properties::setPower(lightId, 50.0f);
    ammonite::lighting::linkModel(lightId, loadedModelIds[modelCount - 1]);
    ammonite::lighting::setAmbientLight(ambientLight);

    //Set the camera position
    const AmmoniteId cameraId = ammonite::camera::getActiveCamera();
    ammonite::camera::setPosition(cameraId, cameraPosition);
    ammonite::camera::setAngle(cameraId, ammonite::radians(180.0f), ammonite::radians(0.0f));

    return true;
  }

  bool rendererMainloop() {
    ammonite::renderer::drawFrame();
    return true;
  }
}
