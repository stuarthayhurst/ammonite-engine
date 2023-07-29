#include <vector>
#include <iostream>

#include <string.h>

#include "../ammonite/ammonite.hpp"

namespace sponzaDemo {
  namespace {
    std::vector<int> loadedModelIds;
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
    int screenId = ammonite::interface::getActiveLoadingScreen();

    //Load models from a set of objects and textures
    const char* models[][2] = {
      {"assets-experimental/intel-assets/main_sponza/NewSponza_Main_glTF_002.gltf", ""},
      {"assets/cube.obj", "assets/flat.png"}
    };
    int modelCount = sizeof(models) / sizeof(models[0]);

    bool success = true;
    long int vertexCount = 0;
    for (int i = 0; i < modelCount; i++) {
      //Load model
      loadedModelIds.push_back(ammonite::models::createModel(models[i][0], &success));

      if (!success) {
        //Prevent total failure if models fail
        success = true;
        continue;
      }

      //Sum vertices and load texture if given
      vertexCount += ammonite::models::getVertexCount(loadedModelIds[i]);
      if (strcmp(models[i][1], "")) {
        ammonite::models::applyTexture(loadedModelIds[i], models[i][1], true, &success);
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
    ammonite::models::position::setPosition(loadedModelIds[modelCount - 1], glm::vec3(4.0f, 4.0f, 4.0f));
    ammonite::models::position::scaleModel(loadedModelIds[modelCount - 1], 0.25f);

    int skyboxId = ammonite::environment::skybox::loadDirectory("assets-experimental/skybox/", &success);
    if (success) {
      ammonite::environment::skybox::setActiveSkybox(skyboxId);
    } else {
      std::cerr << ammonite::utils::warning << "Skybox failed to load" << std::endl;
    }

    std::cout << "STATUS: Loaded " << vertexCount << " vertices" << std::endl;

    //Update loading screen
    ammonite::interface::setLoadingScreenProgress(screenId, 1.0f);
    ammonite::renderer::drawFrame();

    //Set light source properties
    int lightId = ammonite::lighting::createLightSource();
    ammonite::lighting::properties::setPower(lightId, 50.0f);
    ammonite::lighting::linkModel(lightId, loadedModelIds[modelCount - 1]);
    ammonite::lighting::updateLightSources();
    ammonite::lighting::setAmbientLight(glm::vec3(0.1f, 0.1f, 0.1f));

    return 0;
  }

  int rendererMainloop() {
    ammonite::renderer::drawFrame();
    return 0;
  }
}
