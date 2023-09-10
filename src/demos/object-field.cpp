#include <vector>
#include <iostream>
#include <ctime>
#include <cmath>

#include "../ammonite/ammonite.hpp"

namespace objectFieldDemo {
  namespace {
    constexpr bool isWithinThresholdDeg(float angleA, float angleB, float threshold) {
      float delta = angleA - angleB;
      if (delta < -180.0f) {
        delta += 360.0f;
      } else if (delta > 180.0f) {
        delta -= 360.0f;
      }

      delta = std::sqrt(std::pow(delta, 2));
      return (delta <= threshold);
    }

    void genRandomPosData(glm::vec3 objectData[][3], int objectCount) {
      std::srand(std::time(nullptr));
      for (int i = 0; i < objectCount; i++) {
        glm::vec3 position, rotation, scale;

        position.x = (std::rand() / float(((RAND_MAX + 1u) / 20))) - 10.0f;
        position.y = (std::rand() / float(((RAND_MAX + 1u) / 4))) - 3.0f;
        position.z = (std::rand() / float(((RAND_MAX + 1u) / 20))) - 10.0f;

        rotation.x = std::rand() / float(((RAND_MAX + 1u) / 360));
        rotation.y = std::rand() / float(((RAND_MAX + 1u) / 360));
        rotation.z = std::rand() / float(((RAND_MAX + 1u) / 360));

        scale = glm::vec3(std::rand() / float(((RAND_MAX + 1u) / 1.2f)));

        objectData[i][0] = position;
        objectData[i][1] = rotation;
        objectData[i][2] = scale;
      }
    }

    void spawnCubeCallback(int, int, void* userPtr) {
      std::vector<int>* loadedModelIds = (std::vector<int>*)userPtr;

      int activeCameraId = ammonite::camera::getActiveCamera();
      int modelId = ammonite::models::copyModel((*loadedModelIds)[2]);
      loadedModelIds->push_back(modelId);

      float horiz = ammonite::camera::getHorizontal(activeCameraId);
      float vert = ammonite::camera::getVertical(activeCameraId);

      ammonite::models::position::setRotation(modelId, glm::vec3(-vert, horiz, 0.0f));
      ammonite::models::position::setScale(modelId, 0.25f);
      ammonite::models::position::setPosition(modelId, ammonite::camera::getPosition(activeCameraId));

      std::cout << "Spawned object" << std::endl;
    }
  }

  namespace {
    GLFWwindow* windowPtr;

    std::vector<int> loadedModelIds;
    int modelCount = 0;
    const int cubeCount = 30;

    struct LightData {
      ammonite::utils::Timer lightOrbitTimer;
      bool isOrbitClockwise = false;
      bool lastWindowState = false;
      float lightOrbitPeriod;
      int orbitIndex;
      int linkedModelId;
    };
    LightData lightData[2];
    int lightCount = sizeof(lightData) / sizeof(lightData[0]);
  }

  int demoExit() {
    ammonite::input::unregisterKeybind(GLFW_KEY_F);

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
    windowPtr = ammonite::window::getWindowPtr();

    //Hold data for randomised cube positions
    glm::vec3 cubeData[cubeCount + 1][3] = {
      {glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f), glm::vec3(10.0f, 0.1f, 10.0f)},
    };

    //Generate random position, rotation and scales, skip first item
    genRandomPosData(&cubeData[1], cubeCount);

    //Load models from a set of objects and textures
    const char* models[][2] = {
      {"assets/sphere.obj", "assets/flat.png"},
      {"assets/sphere.obj", "assets/flat.png"},
      {"assets/cube.obj", "assets/flat.png"}
    };
    modelCount = sizeof(models) / sizeof(models[0]);
    int totalModels = modelCount + cubeCount;

    bool success = true;
    long int vertexCount = 0;
    for (int i = 0; i < modelCount; i++) {
      //Load model
      loadedModelIds.push_back(ammonite::models::createModel(models[i][0], &success));
      vertexCount += ammonite::models::getVertexCount(loadedModelIds[i]);
      ammonite::models::applyTexture(loadedModelIds[i], AMMONITE_DIFFUSE_TEXTURE, models[i][1], true, &success);

      //Update loading screen
      ammonite::interface::setLoadingScreenProgress(screenId, float(i + 1) / float(totalModels + 1));
      ammonite::renderer::drawFrame();
    }

    if (!success) {
      demoExit();
      return -1;
    }

    //Position the floor
    ammonite::models::position::setPosition(loadedModelIds[2], cubeData[0][0]);
    ammonite::models::position::setRotation(loadedModelIds[2], cubeData[0][1]);
    ammonite::models::position::setScale(loadedModelIds[2], cubeData[0][2]);

    for (int i = 1; i < cubeCount; i++) {
      //Load the cube
      int targetIndex = i + 2;
      loadedModelIds.push_back(ammonite::models::copyModel(loadedModelIds[2]));
      vertexCount += ammonite::models::getVertexCount(loadedModelIds[targetIndex]);
      modelCount++;

      //Position the cube
      ammonite::models::position::setPosition(loadedModelIds[targetIndex], cubeData[i][0]);
      ammonite::models::position::setRotation(loadedModelIds[targetIndex], cubeData[i][1]);
      ammonite::models::position::setScale(loadedModelIds[targetIndex], cubeData[i][2]);

      //Update loading screen
      ammonite::interface::setLoadingScreenProgress(screenId, ((float)modelCount) / ((float)(totalModels) + 1.0f));
      ammonite::renderer::drawFrame();
    }

    std::cout << "STATUS: Loaded " << vertexCount << " vertices" << std::endl;

    //Update loading screen
    ammonite::interface::setLoadingScreenProgress(screenId, 1.0f);
    ammonite::renderer::drawFrame();

    //Create light sources
    int lightIds[2];
    lightIds[0] = ammonite::lighting::createLightSource();
    lightIds[1] = ammonite::lighting::createLightSource();

    //Adjust light model properties
    ammonite::models::position::setPosition(loadedModelIds[0], glm::vec3(4.0f, 4.0f, 4.0f));
    ammonite::models::position::setPosition(loadedModelIds[1], glm::vec3(-4.0f, 4.0f, 4.0f));
    ammonite::models::position::setScale(loadedModelIds[0], 0.1f);
    ammonite::models::position::setScale(loadedModelIds[1], 0.1f);

    //Link to spheres
    ammonite::lighting::properties::setPower(lightIds[0], 50.0f);
    ammonite::lighting::properties::setPower(lightIds[1], 50.0f);
    ammonite::lighting::linkModel(lightIds[0], loadedModelIds[0]);
    ammonite::lighting::linkModel(lightIds[1], loadedModelIds[1]);

    //Update trackers
    ammonite::lighting::updateLightSources();
    ammonite::lighting::setAmbientLight(glm::vec3(0.1f, 0.1f, 0.1f));

    lightData[0].orbitIndex = 0;
    lightData[1].orbitIndex = 1;
    lightData[0].linkedModelId = loadedModelIds[0];
    lightData[1].linkedModelId = loadedModelIds[1];
    lightData[0].lightOrbitPeriod = 2.0f;
    lightData[1].lightOrbitPeriod = 8.0f;

    //Set keybinds
    ammonite::input::registerToggleKeybind(GLFW_KEY_F, spawnCubeCallback, &loadedModelIds);

    return 0;
  }

  int rendererMainloop() {
    static const float lightOrbitRadius = 5.0f;
    static const glm::vec2 lightOrbitNuclei[4] = {
      glm::vec2(-lightOrbitRadius, -lightOrbitRadius),
      glm::vec2( lightOrbitRadius, -lightOrbitRadius),
      glm::vec2(-lightOrbitRadius,  lightOrbitRadius),
      glm::vec2( lightOrbitRadius,  lightOrbitRadius)
    };

    //Structure to store ratios where orbit can change
    static const float swapAngles[4][2] = {
     {360.0f * (0.0f / 4.0f), 360.0f * (3.0f / 4.0f)},
     {360.0f * (2.0f / 4.0f), 360.0f * (3.0f / 4.0f)},
     {360.0f * (0.0f / 4.0f), 360.0f * (1.0f / 4.0f)},
     {360.0f * (1.0f / 4.0f), 360.0f * (2.0f / 4.0f)}
    };

    //Structure to store indices where orbit can change to
    static const float threshold = 1.0f;
    static const int swapTargets[4][2] = {
     {1, 2}, {0, 3}, {3, 0}, {1, 2}
    };

    for (int i = 0; i < lightCount; i++) {
      glm::vec2 lightOrbitNucleus = lightOrbitNuclei[lightData[i].orbitIndex];

      float orbitTime = lightData[i].lightOrbitTimer.getTime();
      if (orbitTime >= lightData[i].lightOrbitPeriod) {
        orbitTime -= lightData[i].lightOrbitPeriod;
        lightData[i].lightOrbitTimer.reset();
      }

      //Use inverse of the time if orbiting backwards
      if (lightData[i].isOrbitClockwise) {
        orbitTime = lightData[i].lightOrbitPeriod - orbitTime;
      }

      //Decide where the light source should be
      float targetAngleDeg = 360.0f * (orbitTime / lightData[i].lightOrbitPeriod);
      float targetAngleRad = glm::radians(targetAngleDeg);

      //Decide if the light is within the region to swap orbits
      int swapTarget = -1;
      int swapWindowNum = 0;
      if (isWithinThresholdDeg(targetAngleDeg, swapAngles[lightData[i].orbitIndex][0], threshold)) {
        swapTarget = swapTargets[lightData[i].orbitIndex][0];
        swapWindowNum = 0;
      }
      if (isWithinThresholdDeg(targetAngleDeg, swapAngles[lightData[i].orbitIndex][1], threshold)) {
        swapTarget = swapTargets[lightData[i].orbitIndex][1];
        swapWindowNum = 1;
      }

      bool isInsideWindow = (swapTarget != -1);
      if (isInsideWindow != lightData[i].lastWindowState) {
        if (isInsideWindow) {
          lightData[i].lastWindowState = true;

          //Randomly decide whether or not to change orbits
          if (rand() > (RAND_MAX / 2)) {
            //Correct current time relative to new orbit
            float swapAngle = swapAngles[lightData[i].orbitIndex][swapWindowNum];
            if (lightData[i].isOrbitClockwise) {
              swapAngle = 360.0f - swapAngle;
            }
            lightData[i].lightOrbitTimer.setTime(((180.0f - swapAngle) / 360.0f) * lightData[i].lightOrbitPeriod);

            //Set new orbit and flip direction
            lightData[i].orbitIndex = swapTarget;
            lightData[i].isOrbitClockwise = !lightData[i].isOrbitClockwise;
          }
        } else {
          lightData[i].lastWindowState = false;
        }
      }

      //Calculate and set final position of light
      float lightPositionX = (lightOrbitRadius * std::cos(targetAngleRad)) + lightOrbitNucleus.x;
      float lightPositionY = (-lightOrbitRadius * std::sin(targetAngleRad)) + lightOrbitNucleus.y;
      ammonite::models::position::setPosition(lightData[i].linkedModelId, glm::vec3(lightPositionX, 4.0f, lightPositionY));
    }

    //Update light and draw the frame
    ammonite::lighting::updateLightSources();
    ammonite::renderer::drawFrame();
    return 0;
  }
}
