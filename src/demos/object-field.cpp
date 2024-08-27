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
      float power = 25.0f;

      //Light/ orbit save data
      ammonite::utils::Timer orbitTimer;
      bool isOrbitClockwise = false;
      bool lastWindowState = false;
      int orbitIndex;
      int linkedModelId;
    };
    LightData* lightData = nullptr;
    int lightCount;

    //General orbit config / data
    float transferProbability = 0.5f;
    int totalOrbits;

    //2D structures to store indices and ratios for orbit changes
    int (*orbitSwapTargets)[2] = nullptr;
    float (*orbitSwapAngles)[2] = nullptr;

    //Model counts
    const int cubeCount = 30;
    int modelCount = 0;
  }

  //Non-orbit internal functions
  namespace {
    static void genRandomPosData(glm::vec3* objectData, int objectCount) {
      for (int i = 0; i < objectCount; i++) {
        //Position;
        objectData[(i * 3) + 0].x = ammonite::utils::randomDouble(-10.0, 10.0);
        objectData[(i * 3) + 0].y = ammonite::utils::randomDouble(-2.0, 1.0);
        objectData[(i * 3) + 0].z = ammonite::utils::randomDouble(-10.0, 10.0);

        //Rotation
        objectData[(i * 3) + 1].x = ammonite::utils::randomDouble(0.0, glm::two_pi<float>());
        objectData[(i * 3) + 1].y = ammonite::utils::randomDouble(0.0, glm::two_pi<float>());
        objectData[(i * 3) + 1].z = ammonite::utils::randomDouble(0.0, glm::two_pi<float>());

        //Scale
        objectData[(i * 3) + 2] = glm::vec3(ammonite::utils::randomDouble(0.0, 1.2));
      }
    }

    static void genCubesCallback(std::vector<int>, int, void*) {
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

    static void spawnCubeCallback(std::vector<int>, int, void*) {
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
    //Return true if 2 angles are within threshold radians of each other
    constexpr static bool isWithinThreshold(float angleA, float angleB, float threshold) {
      float delta = angleA - angleB;
      if (delta < -glm::pi<float>()) {
        delta += glm::two_pi<float>();
      } else if (delta > glm::pi<float>()) {
        delta -= glm::two_pi<float>();
      }

      return (std::abs(delta) <= threshold);
    }

    //Return the position of an orbit's centre, so that each orbit in a shape meets the next
    constexpr static glm::vec2 calculateOrbitPosition(int orbitCount, int orbitIndex,
                                                      float radius) {
      float nucleusAngle = (glm::two_pi<float>() * orbitIndex) / orbitCount;

      //Correct for overlapping orbits
      float indexOffsetAngle = glm::half_pi<float>() - (glm::pi<float>() / orbitCount);
      float opp = glm::pi<float>() - (2 * indexOffsetAngle);
      float nucleusDistance = radius * 2 * std::sin(indexOffsetAngle) / std::sin(opp);

      return nucleusDistance * glm::vec2(std::sin(nucleusAngle), std::cos(nucleusAngle));
    }

    /*
     - Return a 2D array of angles an orbit could swap from, in radians
     - First index has the angle to the previous index
       - The second index has the angle to the next index
    */
    static float* calculateSwapAngles(int orbitCount) {
      const float down = glm::half_pi<float>();
      const float indexOffsetAngle = glm::half_pi<float>() - (glm::pi<float>() / orbitCount);

      float* swapAngles = new float[orbitCount * 2];
      for (int orbit = 0; orbit < orbitCount; orbit++) {
        int writeIndex = orbit * 2;

        //Calculate both directions
        for (int sign = -1; sign <= 1; sign += 2) {
          //Calculate angle to previous / next index
          swapAngles[writeIndex] = down - (indexOffsetAngle * sign);

          //Rotate the angle to match index position
          swapAngles[writeIndex] += (orbit / (float)orbitCount) * glm::two_pi<float>();
          if (swapAngles[writeIndex] >= glm::two_pi<float>()) {
            swapAngles[writeIndex] -= glm::two_pi<float>();
          } else if (swapAngles[writeIndex] <= 0.0f) {
            swapAngles[writeIndex] += glm::two_pi<float>();
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
    static int* calculateSwapTargets(int orbitCount) {
      int* swapTargets = new int[orbitCount * 2];
      for (int orbit = 0; orbit < orbitCount; orbit++) {
        int writeIndex = orbit * 2;
        swapTargets[writeIndex] = (orbit - 1) % orbitCount;
        if (swapTargets[writeIndex] < 0) {
          swapTargets[writeIndex] += orbitCount;
        }
        swapTargets[writeIndex + 1] = (orbit + 1) % orbitCount;
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

    if (lightData != nullptr) {
      delete [] lightData;
      delete [] orbitSwapTargets;
      delete [] orbitSwapAngles;
    }

    return 0;
  }

  int preRendererInit() {
    //Pick number of orbits
    totalOrbits = ammonite::utils::randomInt(3, 8);

    //Pick number of lights
    lightCount = ammonite::utils::randomInt(2, totalOrbits);

    //Allocate and set light data
    lightData = new LightData[lightCount];
    for (int i = 0; i < lightCount; i++) {
      lightData[i].orbitPeriod = 2.0f;
      lightData[i].orbitRadius = 18.0f / totalOrbits;
    }
    lightData[0].orbitPeriod = 8.0f;

    //Fill orbit calculation structures
    orbitSwapTargets = (int(*)[2])calculateSwapTargets(totalOrbits);
    orbitSwapAngles = (float(*)[2])calculateSwapAngles(totalOrbits);

    ammonite::utils::status << "Chose " << totalOrbits << " orbits and " << lightCount \
                            << " lights" << std::endl;

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
      lightData[i].orbitIndex = i % totalOrbits;
    }

    //Set keybinds
    cubeKeybindId = ammonite::input::registerToggleKeybind(GLFW_KEY_F, spawnCubeCallback, nullptr);
    shuffleKeybindId = ammonite::input::registerToggleKeybind(GLFW_KEY_R, genCubesCallback,
                                                              nullptr);

    //Set the camera position
    int cameraId = ammonite::camera::getActiveCamera();
    ammonite::camera::setPosition(cameraId, glm::vec3(7.5f, 7.5f, 7.5f));
    ammonite::camera::setHorizontal(cameraId, 5.0f * glm::quarter_pi<float>());
    ammonite::camera::setVertical(cameraId, -0.3f);

    return 0;
  }

  int rendererMainloop() {
    for (int i = 0; i < lightCount; i++) {
      glm::vec2 lightOrbitPosition = calculateOrbitPosition(totalOrbits,
        lightData[i].orbitIndex, lightData[i].orbitRadius);

      //Snapshot the time, reset timer if it has overrun
      float orbitTime = lightData[i].orbitTimer.getTime();
      if (orbitTime >= lightData[i].orbitPeriod) {
        orbitTime -= lightData[i].orbitPeriod;
        lightData[i].orbitTimer.setTime(orbitTime);
      }

      //Use inverse of the time if orbiting backwards
      if (lightData[i].isOrbitClockwise) {
        orbitTime = lightData[i].orbitPeriod - orbitTime;
      }

      //Decide where the light source's angle, in radians
      float targetAngle = (orbitTime / lightData[i].orbitPeriod) * glm::two_pi<float>();

      //Decide if the light is within the region to swap orbits
      int swapTarget = -1;
      int swapDirection = 0;
      const float threshold = 1.0f / 50.0f;
      if (isWithinThreshold(targetAngle, orbitSwapAngles[lightData[i].orbitIndex][0],
                            threshold)) {
        swapTarget = orbitSwapTargets[lightData[i].orbitIndex][0];
        swapDirection = 0;
      }
      if (isWithinThreshold(targetAngle, orbitSwapAngles[lightData[i].orbitIndex][1],
                            threshold)) {
        swapTarget = orbitSwapTargets[lightData[i].orbitIndex][1];
        swapDirection = 1;
      }

      bool isInsideWindow = (swapTarget != -1);
      if (isInsideWindow != lightData[i].lastWindowState) {
        if (isInsideWindow) {
          lightData[i].lastWindowState = true;

          //Randomly decide whether or not to change orbits
          if (ammonite::utils::randomBool(transferProbability)) {
            //Set timer for new angle
            float newAngle = orbitSwapAngles[swapTarget][1 - swapDirection];
            if (!lightData[i].isOrbitClockwise) {
              newAngle = glm::two_pi<float>() - newAngle;
            }
            lightData[i].orbitTimer.setTime((newAngle / glm::two_pi<float>()) * \
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
      float lightPositionX = (lightData[i].orbitRadius * std::cos(targetAngle)) + \
        lightOrbitPosition.x;
      float lightPositionY = (-lightData[i].orbitRadius * std::sin(targetAngle)) + \
        lightOrbitPosition.y;
      ammonite::models::position::setPosition(lightData[i].linkedModelId,
        glm::vec3(lightPositionX, 5.0f, lightPositionY));
    }

    //Draw the frame
    ammonite::renderer::drawFrame();
    return 0;
  }
}
