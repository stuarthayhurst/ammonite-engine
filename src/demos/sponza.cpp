#include <iostream>
#include <string>
#include <vector>

#include <ammonite/ammonite.hpp>

#include "sponza.hpp"

namespace sponzaDemo {
  namespace {
    std::vector<AmmoniteId> loadedModelIds;

    const ammonite::Vec<float, 3> cameraPosition = {5.0f, 1.5f, 0.0f};
    const ammonite::Vec<float, 3> ambientLight = {0.1f, 0.1f, 0.1f};
    const ammonite::Vec<float, 3> lightModelPosition = {0.0f, 20.0f, 0.0f};
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

    //Load models from a set of objects and materials
    const std::string modelPaths[4] = {
      "assets-experimental/intel-assets/main_sponza/NewSponza_Main_glTF_003.gltf",
      "assets-experimental/intel-assets/pkg_a_curtains/NewSponza_Curtains_glTF.gltf",
      "assets-experimental/intel-assets/pkg_b_ivy/NewSponza_IvyGrowth_glTF.gltf",
      "assets/cube.obj"
    };
    const ammonite::models::AmmoniteMaterial material =
      ammonite::models::createMaterial("assets/flat.png", {0.5f, 0.5f, 0.5f});
    const unsigned int modelCount = sizeof(modelPaths) / sizeof(modelPaths[0]);
    const unsigned int cubeIndex = 3;

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

      //Sum vertices
      vertexCount += ammonite::models::getVertexCount(loadedModelIds[i]);

      //Update splash screen
      ammonite::splash::setSplashScreenProgress(screenId, float(i + 1) / float(modelCount + 1));
      ammonite::renderer::drawFrame();
    }

    //Apply the cube's material
    if (!ammonite::models::applyMaterial(loadedModelIds[cubeIndex], material)) {
      ammonite::utils::warning << "Failed to apply texture '" \
                               << *material.diffuse.textureInfo.texturePath \
                               << "' to '" << modelPaths[cubeIndex] << "'" << std::endl;
    }
    ammonite::models::deleteMaterial(material);

    //Position the light cube
    ammonite::models::position::setPosition(loadedModelIds[cubeIndex], lightModelPosition);
    ammonite::models::position::scaleModel(loadedModelIds[cubeIndex], 0.25f);

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
    ammonite::lighting::setAmbientLight(ambientLight);

    //Set the camera position
    const AmmoniteId cameraId = ammonite::camera::getActiveCamera();
    ammonite::camera::setPosition(cameraId, cameraPosition);
    ammonite::camera::setAngle(cameraId, ammonite::radians(270.0f), ammonite::radians(10.0f));

    return true;
  }

  bool rendererMainloop() {
    ammonite::renderer::drawFrame();
    return true;
  }
}
