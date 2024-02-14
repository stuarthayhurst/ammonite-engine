#include <vector>
#include <iostream>
#include <random>

#include <glm/glm.hpp>

#include "../ammonite/ammonite.hpp"

namespace manyCubesDemo {
  namespace {
    std::vector<int> loadedModelIds;
    int sideLength = 0;

    const int lightCount = 50;
    int lightSourceIds[lightCount];
    glm::vec3 lightSourcePositions[lightCount];
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
    const char* models[2] = {"assets/cube.obj", "assets/flat.png"};
    int modelCount = 10000;

    //Load cube model
    bool success = true;
    loadedModelIds.push_back(ammonite::models::createModel(models[0], &success));
    ammonite::models::applyTexture(loadedModelIds[0], AMMONITE_DIFFUSE_TEXTURE, models[1], true, &success);
    long int vertexCount = ammonite::models::getVertexCount(loadedModelIds[0]);

    if (!success) {
      demoExit();
      return -1;
    }

    for (int i = 1; i < modelCount; i++) {
      //Load model and count vertices
      loadedModelIds.push_back(ammonite::models::copyModel(loadedModelIds[0]));
      vertexCount += ammonite::models::getVertexCount(loadedModelIds[i]);

      //Update loading screen
      ammonite::interface::setLoadingScreenProgress(screenId, float(i + 1) / float(modelCount + 1));
      ammonite::renderer::drawFrame();
    }

    std::cout << "STATUS: Loaded " << vertexCount << " vertices" << std::endl;

    //Update loading screen
    ammonite::interface::setLoadingScreenProgress(screenId, 1.0f);
    ammonite::renderer::drawFrame();

    //Reposition cubes
    sideLength = int(sqrt(modelCount));
    for (int x = 0; x < sideLength; x++) {
      for (int y = 0; y < sideLength; y++) {
        ammonite::models::position::setPosition(loadedModelIds[(x * sideLength) + y], glm::vec3(2.0f * float(x), 0.0f, 2.0f * float(y)));
      }
    }

    //Create light sources
    ammonite::lighting::setAmbientLight(glm::vec3(0.1f, 0.1f, 0.1f));
    for (int i = 0; i < lightCount; i++) {
      lightSourceIds[i] = ammonite::lighting::createLightSource();
      lightSourcePositions[i] = glm::vec3((rand() % sideLength), 4.0f, (rand() % sideLength));
      float red = (rand() % 255 + 0) / 255.0f;
      float green = (rand() % 255 + 0) / 255.0f;
      float blue = (rand() % 255 + 0) / 255.0f;
      ammonite::lighting::properties::setPower(lightSourceIds[i], 50.0f);
      ammonite::lighting::properties::setColour(lightSourceIds[i], glm::vec3(red, green, blue));
    }

    //Set the camera position
    int cameraId = ammonite::camera::getActiveCamera();
    ammonite::camera::setPosition(cameraId, glm::vec3(0.0f, 12.0f, 0.0f));
    ammonite::camera::setHorizontal(cameraId, glm::radians(45.0f));
    ammonite::camera::setVertical(cameraId, glm::radians(-20.0f));

    return 0;
  }

  int rendererMainloop() {
    //Update light source positions
    for (int i = 0; i < lightCount; i++) {
      bool invalid = true;
      int currLightSourceId = lightSourceIds[i];
      float x, z;
      while (invalid) {
        x = (rand() % 200 - 100) / 100.0f;
        z = (rand() % 200 - 100) / 100.0f;

        x += lightSourcePositions[i].x;
        z += lightSourcePositions[i].z;
        if (x >= 0.0f and x <= sideLength) {
          if (z >= 0.0f and z <= sideLength) {
            invalid = false;
          }
        }
      }

      lightSourcePositions[i] = glm::vec3(x, 4.0f, z);
      ammonite::lighting::properties::setGeometry(currLightSourceId, lightSourcePositions[i]);
    }

    ammonite::renderer::drawFrame();
    return 0;
  }
}
