#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include <ammonite/ammonite.hpp>

#include "many-cubes.hpp"

namespace manyCubesDemo {
  namespace {
    std::vector<AmmoniteId> loadedModelIds;
    unsigned int sideLength = 0;

    const int unsigned lightCount = 50;
    AmmoniteId lightSourceIds[lightCount];
    ammonite::Vec<float, 3> lightSourcePositions[lightCount] = {{0}};

    const ammonite::Vec<float, 3> cameraPosition = {0.0f, 12.0f, 0.0f};
    const ammonite::Vec<float, 3> ambientLight = {0.1f, 0.1f, 0.1f};
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

    //Load model from an object and a material
    const ammonite::models::AmmoniteMaterial material =
      ammonite::models::createMaterial("assets/flat.png", {0.5f, 0.5f, 0.5f});
    const std::string modelPath = "assets/cube.obj";
    const unsigned int modelCount = 10000;

    //Load cube model
    const AmmoniteId modelId = ammonite::models::createModel(modelPath);
    loadedModelIds.push_back(modelId);
    const bool applied = ammonite::models::applyMaterial(loadedModelIds[0], material);
    ammonite::models::deleteMaterial(material);
    long unsigned int vertexCount = ammonite::models::getVertexCount(loadedModelIds[0]);
    if (modelId == 0 || !applied) {
      demoExit();
      return false;
    }

    for (unsigned int i = 1; i < modelCount; i++) {
      //Load model and count vertices
      loadedModelIds.push_back(ammonite::models::copyModel(loadedModelIds[0], true));
      vertexCount += ammonite::models::getVertexCount(loadedModelIds[i]);

      //Update splash screen
      const float progress = float(i + 1) / float(modelCount + 1);
      ammonite::splash::setSplashScreenProgress(screenId, progress);
      ammonite::renderer::drawFrame();
    }

    ammonite::utils::status << "Loaded " << vertexCount << " vertices" << std::endl;

    //Update splash screen
    ammonite::splash::setSplashScreenProgress(screenId, 1.0f);
    ammonite::renderer::drawFrame();

    //Reposition cubes
    sideLength = (unsigned int)std::sqrt(modelCount);
    for (unsigned int x = 0; x < sideLength; x++) {
      for (unsigned int y = 0; y < sideLength; y++) {
        const ammonite::Vec<float, 3> position = {2.0f * (float)x, 0.0f, 2.0f * (float)y};
        ammonite::models::position::setPosition(loadedModelIds[((std::size_t)x * sideLength) + y],
          position);
      }
    }

    //Create light sources
    ammonite::lighting::setAmbientLight(ambientLight);
    for (unsigned int i = 0; i < lightCount; i++) {
      lightSourceIds[i] = ammonite::lighting::createLightSource();
      ammonite::set(lightSourcePositions[i],
        ammonite::utils::random<float>((float)sideLength),
        4.0f,
        ammonite::utils::random<float>((float)sideLength));

      const ammonite::Vec<float, 3> colour = {
        ammonite::utils::random<float>(1.0f),
        ammonite::utils::random<float>(1.0f),
        ammonite::utils::random<float>(1.0f)
      };

      ammonite::lighting::properties::setPower(lightSourceIds[i], 50.0f);
      ammonite::lighting::properties::setColour(lightSourceIds[i], colour);
    }

    //Set the camera position
    const AmmoniteId cameraId = ammonite::camera::getActiveCamera();
    ammonite::camera::setPosition(cameraId, cameraPosition);
    ammonite::camera::setAngle(cameraId, ammonite::radians(45.0f), ammonite::radians(-20.0f));

    return true;
  }

  bool rendererMainloop() {
    //Update light source positions
    for (unsigned int i = 0; i < lightCount; i++) {
      //Apply random offset
      const ammonite::Vec<float, 3> offset = {
        ammonite::utils::random<float>(-1.0f, 1.0f),
        0.0f,
        ammonite::utils::random<float>(-1.0f, 1.0f)
      };
      ammonite::add(lightSourcePositions[i], offset);

      //Clamp to bounds
      lightSourcePositions[i][0] = std::clamp(lightSourcePositions[i][0], 0.0f, (float)sideLength);
      lightSourcePositions[i][1] = 4.0f;
      lightSourcePositions[i][2] = std::clamp(lightSourcePositions[i][2], 0.0f, (float)sideLength);

      ammonite::lighting::properties::setGeometry(lightSourceIds[i], lightSourcePositions[i]);
    }

    ammonite::renderer::drawFrame();
    return true;
  }
}
