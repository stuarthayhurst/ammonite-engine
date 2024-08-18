#include <vector>
#include <iostream>
#include <ctime>
#include <cmath>

#include "../ammonite/ammonite.hpp"

namespace objectFieldDemo {
  namespace {
    //IDs and pointer data
    GLFWwindow* windowPtr;
    int cubeKeybindId;
    int shuffleKeybindId;
    std::vector<int> loadedModelIds;
    int floorId;

    struct LightData {
      //Light / orbit config
      float orbitPeriod;
      float orbitRadius;
      float scale;
      float power;

      //Light/ orbit save data
      ammonite::utils::Timer orbitTimer;
      bool isOrbitClockwise = false;
      bool lastWindowState = false;
      int orbitIndex;
      int linkedModelId;
    } lightData[2];

    //Model counts
    const int lightCount = sizeof(lightData) / sizeof(lightData[0]);
    const int cubeCount = 30;
    int modelCount = 0;
  }

  //Non-orbit internal functions
  namespace {
    void genRandomPosData(glm::vec3* objectData, int objectCount) {
      for (int i = 0; i < objectCount; i++) {
        glm::vec3 position, rotation, scale;

        position.x = (std::rand() / float(((RAND_MAX + 1u) / 20))) - 10.0f;
        position.y = (std::rand() / float(((RAND_MAX + 1u) / 3))) - 2.0f;
        position.z = (std::rand() / float(((RAND_MAX + 1u) / 20))) - 10.0f;

        rotation.x = std::rand() / float(((RAND_MAX + 1u) / 360));
        rotation.y = std::rand() / float(((RAND_MAX + 1u) / 360));
        rotation.z = std::rand() / float(((RAND_MAX + 1u) / 360));

        scale = glm::vec3(std::rand() / float(((RAND_MAX + 1u) / 1.2f)));

        objectData[(i * 3) + 0] = position;
        objectData[(i * 3) + 1] = rotation;
        objectData[(i * 3) + 2] = scale;
      }
    }

    void genCubesCallback(std::vector<int>, int, void*) {
      //Hold data for randomised cube positions
      int offset = lightCount + 1;
      int cubeCount = loadedModelIds.size() - offset;
      glm::vec3* cubeData = new glm::vec3[cubeCount * 3];

      //Generate random position, rotation and scales, skip first item
      genRandomPosData(cubeData, cubeCount);

      for (int i = 0; i < cubeCount; i++) {
        //Position the cube
        ammonite::models::position::setPosition(loadedModelIds[i + offset], cubeData[(i * 3) + 0]);
        ammonite::models::position::setRotation(loadedModelIds[i + offset], cubeData[(i * 3) + 1]);
        ammonite::models::position::setScale(loadedModelIds[i + offset], cubeData[(i * 3) + 2]);
      }

      delete [] cubeData;
      ammonite::utils::status << "Shuffled cubes" << std::endl;
    }

    void spawnCubeCallback(std::vector<int>, int, void*) {
      int activeCameraId = ammonite::camera::getActiveCamera();
      int modelId = ammonite::models::copyModel(floorId);
      loadedModelIds.push_back(modelId);

      float horiz = ammonite::camera::getHorizontal(activeCameraId);
      float vert = ammonite::camera::getVertical(activeCameraId);

      ammonite::models::position::setRotation(modelId, glm::vec3(-vert, horiz, 0.0f));
      ammonite::models::position::setScale(modelId, 0.25f);
      ammonite::models::position::setPosition(modelId,
                                              ammonite::camera::getPosition(activeCameraId));

      ammonite::utils::status << "Spawned object" << std::endl;
    }
  }

  //Orbit handling internal functions
  namespace {
    constexpr static bool isWithinThresholdDeg(float angleA, float angleB, float threshold) {
      float delta = angleA - angleB;
      if (delta < -180.0f) {
        delta += 360.0f;
      } else if (delta > 180.0f) {
        delta -= 360.0f;
      }

      delta = std::sqrt(std::pow(delta, 2));
      return (delta <= threshold);
    }

    constexpr static glm::vec2 calculateOrbitNucleus(int orbitIndex, float radius) {
      const glm::vec2 lightOrbitNuclei[4] = {
        glm::vec2(-radius, -radius),
        glm::vec2( radius, -radius),
        glm::vec2(-radius,  radius),
        glm::vec2( radius,  radius)
      };

      return lightOrbitNuclei[orbitIndex];
    }
  }

  int demoExit() {
    ammonite::input::unregisterKeybind(cubeKeybindId);
    ammonite::input::unregisterKeybind(shuffleKeybindId);

    for (unsigned int i = 0; i < loadedModelIds.size(); i++) {
      ammonite::models::deleteModel(loadedModelIds[i]);
    }

    return 0;
  }

  int preRendererInit() {
    //Set up light config
    lightData[0].orbitPeriod = 2.0f;
    lightData[0].orbitRadius = 5.0f;
    lightData[0].scale = 0.1f;
    lightData[0].power = 50.0f;

    lightData[1].orbitPeriod = 8.0f;
    lightData[1].orbitRadius = 5.0f;
    lightData[1].scale = 0.1f;
    lightData[1].power = 50.0f;

    //Prepare the random number generator
    std::srand(std::time(nullptr));

    return 0;
  }

  int postRendererInit() {
    int screenId = ammonite::interface::getActiveLoadingScreen();
    windowPtr = ammonite::window::getWindowPtr();

    //Generate random positions, orientations and sizes, skipping first item
    glm::vec3 cubeData[cubeCount][3];
    genRandomPosData(&cubeData[0][0], cubeCount);

    //Load models from a set of objects and textures
    const char* models[][2] = {
      {"assets/sphere.obj", "assets/flat.png"},
      {"assets/cube.obj", "assets/flat.png"}
    };
    int totalModels = lightCount + cubeCount + 1;

    //Load light models
    bool success = true;
    long int vertexCount = 0;
    for (int i = 0; i < lightCount; i++) {
      //Load model
      loadedModelIds.push_back(ammonite::models::createModel(models[0][0], &success));
      vertexCount += ammonite::models::getVertexCount(loadedModelIds[i]);
      ammonite::models::applyTexture(loadedModelIds[i], AMMONITE_DIFFUSE_TEXTURE,
                                     models[0][1], true, &success);

      //Update loading screen
      modelCount++;
      ammonite::interface::setLoadingScreenProgress(screenId,
        float(modelCount) / float(totalModels));
      ammonite::renderer::drawFrame();
    }

    //Load the floor
    floorId = ammonite::models::createModel(models[1][0], &success);
    loadedModelIds.push_back(floorId);
    vertexCount += ammonite::models::getVertexCount(floorId);
    ammonite::models::applyTexture(floorId, AMMONITE_DIFFUSE_TEXTURE, models[1][1],
                                   true, &success);
    modelCount++;

    if (!success) {
      demoExit();
      return -1;
    }

    //Position the floor
    ammonite::models::position::setPosition(floorId, glm::vec3(0.0f, -1.0f, 0.0f));
    ammonite::models::position::setRotation(floorId, glm::vec3(0.0f));
    ammonite::models::position::setScale(floorId, glm::vec3(10.0f, 0.1f, 10.0f));

    for (int i = 0; i < cubeCount; i++) {
      //Load the cube
      loadedModelIds.push_back(ammonite::models::copyModel(floorId));
      vertexCount += ammonite::models::getVertexCount(loadedModelIds[modelCount]);

      //Position the cube
      ammonite::models::position::setPosition(loadedModelIds[modelCount], cubeData[i][0]);
      ammonite::models::position::setRotation(loadedModelIds[modelCount], cubeData[i][1]);
      ammonite::models::position::setScale(loadedModelIds[modelCount], cubeData[i][2]);

      //Update loading screen
      modelCount++;
      ammonite::interface::setLoadingScreenProgress(screenId,
        (float)modelCount / (float)totalModels);
      ammonite::renderer::drawFrame();
    }

    ammonite::utils::status << "Loaded " << vertexCount << " vertices" << std::endl;

    //Update loading screen
    ammonite::interface::setLoadingScreenProgress(screenId, 1.0f);
    ammonite::renderer::drawFrame();

    //Setup each light and model
    ammonite::lighting::setAmbientLight(glm::vec3(0.1f, 0.1f, 0.1f));
    for (int i = 0; i < lightCount; i++) {
      int lightId = ammonite::lighting::createLightSource();
      ammonite::models::position::setScale(loadedModelIds[i], lightData[i].scale);
      ammonite::lighting::properties::setPower(lightId, lightData[i].power);
      ammonite::lighting::linkModel(lightId, loadedModelIds[i]);
      lightData[i].linkedModelId = loadedModelIds[i];
    }

    lightData[0].orbitIndex = 0;
    lightData[1].orbitIndex = 1;

    //Set keybinds
    cubeKeybindId = ammonite::input::registerToggleKeybind(GLFW_KEY_F, spawnCubeCallback, nullptr);
    shuffleKeybindId = ammonite::input::registerToggleKeybind(GLFW_KEY_R, genCubesCallback,
                                                              nullptr);

    //Set the camera position
    int cameraId = ammonite::camera::getActiveCamera();
    ammonite::camera::setPosition(cameraId, glm::vec3(7.5f, 7.5f, 7.5f));
    ammonite::camera::setHorizontal(cameraId, glm::radians(225.0f));
    ammonite::camera::setVertical(cameraId, glm::radians(-20.0f));

    return 0;
  }

  int rendererMainloop() {
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
      glm::vec2 lightOrbitNucleus = calculateOrbitNucleus(lightData[i].orbitIndex,
        lightData[i].orbitRadius);

      float orbitTime = lightData[i].orbitTimer.getTime();
      if (orbitTime >= lightData[i].orbitPeriod) {
        orbitTime -= lightData[i].orbitPeriod;
        lightData[i].orbitTimer.reset();
      }

      //Use inverse of the time if orbiting backwards
      if (lightData[i].isOrbitClockwise) {
        orbitTime = lightData[i].orbitPeriod - orbitTime;
      }

      //Decide where the light source should be
      float targetAngleDeg = 360.0f * (orbitTime / lightData[i].orbitPeriod);
      float targetAngleRad = glm::radians(targetAngleDeg);

      //Decide if the light is within the region to swap orbits
      int swapTarget = -1;
      int swapWindowNum = 0;
      if (isWithinThresholdDeg(targetAngleDeg, swapAngles[lightData[i].orbitIndex][0],
                               threshold)) {
        swapTarget = swapTargets[lightData[i].orbitIndex][0];
        swapWindowNum = 0;
      }
      if (isWithinThresholdDeg(targetAngleDeg, swapAngles[lightData[i].orbitIndex][1],
                               threshold)) {
        swapTarget = swapTargets[lightData[i].orbitIndex][1];
        swapWindowNum = 1;
      }

      bool isInsideWindow = (swapTarget != -1);
      if (isInsideWindow != lightData[i].lastWindowState) {
        if (isInsideWindow) {
          lightData[i].lastWindowState = true;

          //Randomly decide whether or not to change orbits
          if (std::rand() > (RAND_MAX / 2)) {
            //Correct current time relative to new orbit
            float swapAngle = swapAngles[lightData[i].orbitIndex][swapWindowNum];
            if (lightData[i].isOrbitClockwise) {
              swapAngle = 360.0f - swapAngle;
            }
            lightData[i].orbitTimer.setTime(((180.0f - swapAngle) / 360.0f) * \
              lightData[i].orbitPeriod);

            //Set new orbit and flip direction
            lightData[i].orbitIndex = swapTarget;
            lightData[i].isOrbitClockwise = !lightData[i].isOrbitClockwise;
          }
        } else {
          lightData[i].lastWindowState = false;
        }
      }

      //Calculate and set final position of light
      float lightPositionX = (lightData[i].orbitRadius * \
        std::cos(targetAngleRad)) + lightOrbitNucleus.x;
      float lightPositionY = (-lightData[i].orbitRadius * \
        std::sin(targetAngleRad)) + lightOrbitNucleus.y;
      ammonite::models::position::setPosition(lightData[i].linkedModelId,
        glm::vec3(lightPositionX, 4.0f, lightPositionY));
    }

    //Draw the frame
    ammonite::renderer::drawFrame();
    return 0;
  }
}
