#include <vector>
#include <iostream>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

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
      float scale = 0.1f;
      float power = 40.0f;

      //Light/ orbit save data
      ammonite::utils::Timer orbitTimer;
      bool isOrbitClockwise = false;
      bool lastWindowState = false;
      int orbitIndex;
      int linkedModelId;
    } lightData[2];

    //Number of orbits
    int totalNuclei = 4;

    //2D structures to store indices and ratios for orbit changes
    int (*orbitSwapTargets)[2] = nullptr;
    float (*orbitSwapAngles)[2] = nullptr;

    //Model counts
    const int lightCount = sizeof(lightData) / sizeof(lightData[0]);
    const int cubeCount = 30;
    int modelCount = 0;
  }

  //Non-orbit internal functions
  namespace {
    void genRandomPosData(glm::vec3* objectData, int objectCount) {
      for (int i = 0; i < objectCount; i++) {
        //Position;
        objectData[(i * 3) + 0].x = ammonite::utils::randomDouble(-10.0, 10.0);
        objectData[(i * 3) + 0].y = ammonite::utils::randomDouble(-2.0, 1.0);
        objectData[(i * 3) + 0].z = ammonite::utils::randomDouble(-10.0, 10.0);

        //Rotation
        objectData[(i * 3) + 1].x = ammonite::utils::randomDouble(0.0, 360.0);
        objectData[(i * 3) + 1].y = ammonite::utils::randomDouble(0.0, 360.0);
        objectData[(i * 3) + 1].z = ammonite::utils::randomDouble(0.0, 360.0);

        //Scale
        objectData[(i * 3) + 2] = glm::vec3(ammonite::utils::randomDouble(0.0, 1.2));
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

      delta = std::abs(delta);
      return (delta <= threshold);
    }

    constexpr static glm::vec2 calculateOrbitNucleus(int nucleusCount,
                                                     int orbitIndex, float radius) {
      float nucleusAngle = glm::radians((360.0f * orbitIndex) / nucleusCount);

      //Correct for overlapping orbits
      float indexOffsetAngle = glm::radians(90.0f - (180.0f / nucleusCount));
      float opp = glm::pi<float>() - (2 * indexOffsetAngle);
      float nucleusDistance = radius * 2 * std::sin(indexOffsetAngle) / std::sin(opp);

      return nucleusDistance * glm::vec2(std::sin(nucleusAngle), std::cos(nucleusAngle));
    }

    /*
     - Return a 2D array of angles an orbit could swap from
     - First index has the angle to the previous index
       - The second index has the angle to the next index
    */
    static float* calculateSwapAngles(int nucleusCount) {
      const float down = 90.0f;
      const float indexOffsetAngle = 90.0f - (180.0f / nucleusCount);

      float* swapAngles = new float[nucleusCount * 2];
      for (int nucleus = 0; nucleus < nucleusCount; nucleus++) {
        int writeIndex = nucleus * 2;

        //Calculate both directions
        for (int sign = -1; sign <= 1; sign += 2) {
          //Calculate angle to previous / next index
          swapAngles[writeIndex] = down - (indexOffsetAngle * sign);

          //Rotate the angle to match index position
          swapAngles[writeIndex] += (nucleus / (float)nucleusCount) * 360.0f;
          if (swapAngles[writeIndex] >= 360.0f) {
            swapAngles[writeIndex] -= 360.0f;
          } else if (swapAngles[writeIndex] <= 0.0f) {
            swapAngles[writeIndex] += 360.0f;
          }

          writeIndex++;
        }
      }

      return swapAngles;
    }

    /*
     - Return a 2D array of indicies an orbit could swap to
     - First index points to previous index
       - The second index points to the next
    */
    static int* calculateSwapTargets(int nucleusCount) {
      int* swapTargets = new int[nucleusCount * 2];
      for (int nucleus = 0; nucleus < nucleusCount; nucleus++) {
        int writeIndex = nucleus * 2;
        swapTargets[writeIndex] = (nucleus - 1) % nucleusCount;
        if (swapTargets[writeIndex] < 0) {
          swapTargets[writeIndex] += nucleusCount;
        }
        swapTargets[writeIndex + 1] = (nucleus + 1) % nucleusCount;
      }

      return swapTargets;
    }
  }

  int demoExit() {
    ammonite::input::unregisterKeybind(cubeKeybindId);
    ammonite::input::unregisterKeybind(shuffleKeybindId);

    for (unsigned int i = 0; i < loadedModelIds.size(); i++) {
      ammonite::models::deleteModel(loadedModelIds[i]);
    }

    if (orbitSwapTargets != nullptr) {
      delete [] orbitSwapTargets;
      delete [] orbitSwapAngles;
    }

    return 0;
  }

  int preRendererInit() {
    //Set up light config
    lightData[0].orbitPeriod = 2.0f;
    lightData[0].orbitRadius = 5.0f;

    lightData[1].orbitPeriod = 8.0f;
    lightData[1].orbitRadius = 5.0f;

    //Fill orbit calculation structures
    orbitSwapTargets = (int(*)[2])calculateSwapTargets(totalNuclei);
    orbitSwapAngles = (float(*)[2])calculateSwapAngles(totalNuclei);

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

    //Place lights on each orbit
    for (int i = 0; i < lightCount; i++) {
      lightData[i].orbitIndex = i % totalNuclei;
    }

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
    for (int i = 0; i < lightCount; i++) {
      glm::vec2 lightOrbitNucleus = calculateOrbitNucleus(totalNuclei,
        lightData[i].orbitIndex, lightData[i].orbitRadius);

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
      int swapDirection = 0;
      static const float thresholdDeg = 1.0f;
      if (isWithinThresholdDeg(targetAngleDeg, orbitSwapAngles[lightData[i].orbitIndex][0],
                               thresholdDeg)) {
        swapTarget = orbitSwapTargets[lightData[i].orbitIndex][0];
        swapDirection = 0;
      }
      if (isWithinThresholdDeg(targetAngleDeg, orbitSwapAngles[lightData[i].orbitIndex][1],
                               thresholdDeg)) {
        swapTarget = orbitSwapTargets[lightData[i].orbitIndex][1];
        swapDirection = 1;
      }

      bool isInsideWindow = (swapTarget != -1);
      if (isInsideWindow != lightData[i].lastWindowState) {
        if (isInsideWindow) {
          lightData[i].lastWindowState = true;

          //Randomly decide whether or not to change orbits
          if (ammonite::utils::randomBool(0.5)) {
            //Set timer for new angle
            float newAngle = orbitSwapAngles[swapTarget][1 - swapDirection];
            if (!lightData[i].isOrbitClockwise) {
              newAngle = 360.0f - newAngle;
            }
            lightData[i].orbitTimer.setTime((newAngle / 360.0f) * lightData[i].orbitPeriod);

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
