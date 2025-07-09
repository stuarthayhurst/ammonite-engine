#include <cstddef>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include <ammonite/ammonite.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "object-field.hpp"

namespace objectFieldDemo {
  namespace {
    //IDs and pointer data
    AmmoniteId cubeKeybindId;
    AmmoniteId shuffleKeybindId;
    std::vector<AmmoniteId> loadedModelIds;
    AmmoniteId floorId;

    struct LightData {
      //Light / orbit config
      float orbitPeriod;
      float orbitRadius;
      float scale = 0.1f;
      float power = 30.0f;

      //Light/ orbit save data
      ammonite::utils::Timer orbitTimer;
      bool isOrbitClockwise = false;
      bool lastWindowState = false;
      unsigned int orbitIndex;
      AmmoniteId linkedModelId;
    };
    LightData* lightData = nullptr;
    unsigned int lightCount;

    //General orbit config / data
    float transferProbability = 0.5f;
    unsigned int totalOrbits;

    //2D structures to store indices and ratios for orbit changes
    unsigned int (*orbitSwapTargets)[2] = nullptr;
    float (*orbitSwapAngles)[2] = nullptr;

    //Model counts
    const unsigned int cubeCount = 30;
    unsigned int modelCount = 0;
  }

  //Non-orbit internal functions
  namespace {
    void genRandomPosData(glm::vec3* objectData, unsigned int objectCount) {
      for (unsigned int i = 0; i < objectCount; i++) {
        //Position;
        objectData[(i * 3) + 0].x = (float)ammonite::utils::randomDouble(-10.0, 10.0);
        objectData[(i * 3) + 0].y = (float)ammonite::utils::randomDouble(-2.0, 1.0);
        objectData[(i * 3) + 0].z = (float)ammonite::utils::randomDouble(-10.0, 10.0);

        //Rotation
        objectData[(i * 3) + 1].x = (float)ammonite::utils::randomDouble(0.0, ammonite::pi<float>() * 2.0f);
        objectData[(i * 3) + 1].y = (float)ammonite::utils::randomDouble(0.0, ammonite::pi<float>() * 2.0f);
        objectData[(i * 3) + 1].z = (float)ammonite::utils::randomDouble(0.0, ammonite::pi<float>() * 2.0f);

        //Scale
        objectData[(i * 3) + 2] = glm::vec3((float)ammonite::utils::randomDouble(0.0, 1.2));
      }
    }

    void genCubesCallback(const std::vector<AmmoniteKeycode>&, KeyStateEnum, void*) {
      //Hold data for randomised cube positions
      const unsigned int offset = lightCount + 1;
      const unsigned int cubeCount = loadedModelIds.size() - offset;
      glm::vec3* cubeData = new glm::vec3[(std::size_t)(cubeCount) * 3];

      //Generate random position, rotation and scales, skip first item
      genRandomPosData(cubeData, cubeCount);

      for (unsigned int i = 0; i < cubeCount; i++) {
        //Position the cube
        ammonite::models::position::setPosition(loadedModelIds[(std::size_t)i + offset],
          cubeData[(i * 3) + 0]);
        ammonite::models::position::setRotation(loadedModelIds[(std::size_t)i + offset],
          cubeData[(i * 3) + 1]);
        ammonite::models::position::setScale(loadedModelIds[(std::size_t)i + offset],
          cubeData[(i * 3) + 2]);
      }

      delete [] cubeData;
      ammonite::utils::status << "Shuffled cubes" << std::endl;
    }

    void spawnCubeCallback(const std::vector<AmmoniteKeycode>&, KeyStateEnum, void*) {
      const AmmoniteId activeCameraId = ammonite::camera::getActiveCamera();
      const AmmoniteId modelId = ammonite::models::copyModel(floorId);
      loadedModelIds.push_back(modelId);

      const double horiz = ammonite::camera::getHorizontal(activeCameraId);
      const double vert = ammonite::camera::getVertical(activeCameraId);

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
    constexpr bool isWithinThreshold(float angleA, float angleB, float threshold) {
      float delta = angleA - angleB;
      if (delta < -ammonite::pi<float>()) {
        delta += ammonite::pi<float>() * 2.0f;
      } else if (delta > ammonite::pi<float>()) {
        delta -= ammonite::pi<float>() * 2.0f;
      }

      return (std::abs(delta) <= threshold);
    }

    //Return the position of an orbit's centre, so that each orbit in a shape meets the next
    constexpr glm::vec2 calculateOrbitPosition(unsigned int orbitCount,
                                               unsigned int orbitIndex, float radius) {
      const float nucleusAngle = (ammonite::pi<float>() * 2.0f * (float)orbitIndex) / (float)orbitCount;

      //Correct for overlapping orbits
      const float indexOffsetAngle = (ammonite::pi<float>() / 2.0f) - (ammonite::pi<float>() / (float)orbitCount);
      const float opp = ammonite::pi<float>() - (2 * indexOffsetAngle);
      const float nucleusDistance = radius * 2 * std::sin(indexOffsetAngle) / std::sin(opp);

      return nucleusDistance * glm::vec2(std::sin(nucleusAngle), std::cos(nucleusAngle));
    }

    /*
     - Return a 2D array of angles an orbit could swap from, in radians
     - First index has the angle to the previous index
       - The second index has the angle to the next index
    */
    float* calculateSwapAngles(unsigned int orbitCount) {
      const float down = ammonite::pi<float>() / 2.0f;
      const float indexOffsetAngle = (ammonite::pi<float>() / 2.0f) - (ammonite::pi<float>() / (float)orbitCount);

      float* swapAngles = new float[(std::size_t)(orbitCount) * 2];
      for (unsigned int orbit = 0; orbit < orbitCount; orbit++) {
        unsigned int writeIndex = orbit * 2;

        //Calculate both directions
        for (int sign = -1; sign <= 1; sign += 2) {
          //Calculate angle to previous / next index
          swapAngles[writeIndex] = down - (indexOffsetAngle * (float)sign);

          //Rotate the angle to match index position
          swapAngles[writeIndex] += ((float)orbit / (float)orbitCount) * ammonite::pi<float>() * 2.0f;
          if (swapAngles[writeIndex] >= ammonite::pi<float>() * 2.0f) {
            swapAngles[writeIndex] -= ammonite::pi<float>() * 2.0f;
          } else if (swapAngles[writeIndex] <= 0.0f) {
            swapAngles[writeIndex] += ammonite::pi<float>() * 2.0f;
          }

          writeIndex++;
        }
      }

      return swapAngles;
    }

    /*
     - Return a 2D array of indices an orbit could swap to
     - The first index of the pair points to previous swap target index
     - The second index of the pair points to the next swap target index
    */
    unsigned int* calculateSwapTargets(unsigned int orbitCount) {
      unsigned int* swapTargets = new unsigned int[(std::size_t)(orbitCount) * 2];
      for (unsigned int orbit = 0; orbit < orbitCount; orbit++) {
        const unsigned int writeIndex = orbit * 2;

        //Find and store the previous index
        if (orbit == 0) {
          swapTargets[writeIndex] = orbitCount - 1;
        } else {
          swapTargets[writeIndex] = orbit - 1;
        }

        //Find and store the next index
        swapTargets[writeIndex + 1] = (orbit + 1) % orbitCount;
      }

      return swapTargets;
    }
  }

  bool demoExit() {
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

    return true;
  }

  bool preEngineInit() {
    //Pick number of orbits
    totalOrbits = ammonite::utils::randomInt(3, 8);

    //Pick number of lights
    lightCount = ammonite::utils::randomInt(2, totalOrbits);

    //Allocate and set light data
    lightData = new LightData[lightCount];
    for (unsigned int i = 0; i < lightCount; i++) {
      lightData[i].orbitPeriod = 2.0f;
      lightData[i].orbitRadius = 18.0f / (float)totalOrbits;
    }
    lightData[0].orbitPeriod = 8.0f;

    //Fill orbit calculation structures
    orbitSwapTargets = (unsigned int(*)[2])calculateSwapTargets(totalOrbits);
    orbitSwapAngles = (float(*)[2])calculateSwapAngles(totalOrbits);

    ammonite::utils::status << "Chose " << totalOrbits << " orbits and " << lightCount \
                            << " lights" << std::endl;

    return true;
  }

  bool postEngineInit() {
    const AmmoniteId screenId = ammonite::splash::getActiveSplashScreenId();

    //Generate random positions, orientations and sizes, skipping first item
    glm::vec3 cubeData[cubeCount][3];
    genRandomPosData(&cubeData[0][0], cubeCount);

    //Load models from a set of objects and textures
    const std::string models[][2] = {
      {"assets/sphere.obj", "assets/flat.png"},
      {"assets/cube.obj", "assets/flat.png"}
    };
    const unsigned int totalModels = lightCount + cubeCount + 1;

    //Load light models
    bool success = true;
    long unsigned int vertexCount = 0;
    for (unsigned int i = 0; i < lightCount; i++) {
      //Load model
      const AmmoniteId modelId = ammonite::models::createModel(models[0][0]);
      loadedModelIds.push_back(modelId);
      vertexCount += ammonite::models::getVertexCount(loadedModelIds[i]);
      success &= ammonite::models::applyTexture(loadedModelIds[i],
        AMMONITE_DIFFUSE_TEXTURE, models[0][1], true);

      if (modelId == 0) {
        success = false;
      }

      //Update splash screen
      modelCount++;
      ammonite::splash::setSplashScreenProgress(screenId,
        float(modelCount) / float(totalModels));
      ammonite::renderer::drawFrame();
    }

    //Load the floor
    floorId = ammonite::models::createModel(models[1][0]);
    loadedModelIds.push_back(floorId);
    vertexCount += ammonite::models::getVertexCount(floorId);
    success &= ammonite::models::applyTexture(floorId, AMMONITE_DIFFUSE_TEXTURE,
                                              models[1][1], true);
    modelCount++;

    if (floorId == 0 || !success) {
      demoExit();
      return false;
    }

    //Position the floor
    ammonite::models::position::setPosition(floorId, glm::vec3(0.0f, -1.0f, 0.0f));
    ammonite::models::position::setRotation(floorId, glm::vec3(0.0f));
    ammonite::models::position::setScale(floorId, glm::vec3(10.0f, 0.1f, 10.0f));

    for (unsigned int i = 0; i < cubeCount; i++) {
      //Load the cube
      loadedModelIds.push_back(ammonite::models::copyModel(floorId));
      vertexCount += ammonite::models::getVertexCount(loadedModelIds[modelCount]);

      //Position the cube
      ammonite::models::position::setPosition(loadedModelIds[modelCount], cubeData[i][0]);
      ammonite::models::position::setRotation(loadedModelIds[modelCount], cubeData[i][1]);
      ammonite::models::position::setScale(loadedModelIds[modelCount], cubeData[i][2]);

      //Update splash screen
      modelCount++;
      ammonite::splash::setSplashScreenProgress(screenId,
        (float)modelCount / (float)totalModels);
      ammonite::renderer::drawFrame();
    }

    ammonite::utils::status << "Loaded " << vertexCount << " vertices" << std::endl;

    //Update splash screen
    ammonite::splash::setSplashScreenProgress(screenId, 1.0f);
    ammonite::renderer::drawFrame();

    //Setup each light and model
    ammonite::lighting::setAmbientLight(glm::vec3(0.1f, 0.1f, 0.1f));
    for (unsigned int i = 0; i < lightCount; i++) {
      const AmmoniteId lightId = ammonite::lighting::createLightSource();
      ammonite::models::position::setScale(loadedModelIds[i], lightData[i].scale);
      ammonite::lighting::properties::setPower(lightId, lightData[i].power);
      ammonite::lighting::linkModel(lightId, loadedModelIds[i]);
      lightData[i].linkedModelId = loadedModelIds[i];
    }

    //Place lights on each orbit
    for (unsigned int i = 0; i < lightCount; i++) {
      lightData[i].orbitIndex = i % totalOrbits;
    }

    //Set keybinds
    cubeKeybindId = ammonite::input::registerToggleKeybind(AMMONITE_F, spawnCubeCallback, nullptr);
    shuffleKeybindId = ammonite::input::registerToggleKeybind(AMMONITE_R, genCubesCallback,
                                                              nullptr);

    //Set the camera position
    const AmmoniteId cameraId = ammonite::camera::getActiveCamera();
    ammonite::camera::setPosition(cameraId, glm::vec3(10.0f, 17.0f, 17.0f));
    ammonite::camera::setAngle(cameraId, 4.75f * ammonite::pi<float>() / 4.0f, -0.7f);

    return true;
  }

  bool rendererMainloop() {
    for (unsigned int i = 0; i < lightCount; i++) {
      const glm::vec2 lightOrbitPosition = calculateOrbitPosition(totalOrbits,
        lightData[i].orbitIndex, lightData[i].orbitRadius);

      //Snapshot the time, reset timer if it has overrun
      float orbitTime = (float)lightData[i].orbitTimer.getTime();
      if (orbitTime >= lightData[i].orbitPeriod) {
        orbitTime -= lightData[i].orbitPeriod;
        lightData[i].orbitTimer.setTime(orbitTime);
      }

      //Use inverse of the time if orbiting backwards
      if (lightData[i].isOrbitClockwise) {
        orbitTime = lightData[i].orbitPeriod - orbitTime;
      }

      //Decide where the light source's angle, in radians
      const float targetAngle = (orbitTime / lightData[i].orbitPeriod) * ammonite::pi<float>() * 2.0f;

      //Decide if the light is within the region to swap orbits
      unsigned int swapTarget = 0;
      unsigned int swapDirection = 2;
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

      const bool isInsideWindow = (swapDirection != 2);
      if (isInsideWindow != lightData[i].lastWindowState) {
        if (isInsideWindow) {
          lightData[i].lastWindowState = true;

          //Randomly decide whether or not to change orbits
          if (ammonite::utils::randomBool(transferProbability)) {
            //Set timer for new angle
            float newAngle = orbitSwapAngles[swapTarget][1 - swapDirection];
            if (!lightData[i].isOrbitClockwise) {
              newAngle = (ammonite::pi<float>() * 2.0f) - newAngle;
            }
            lightData[i].orbitTimer.setTime((newAngle / (ammonite::pi<float>() * 2.0f)) * \
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
      const float lightPositionX = (lightData[i].orbitRadius * std::cos(targetAngle)) + \
        lightOrbitPosition.x;
      const float lightPositionY = (-lightData[i].orbitRadius * std::sin(targetAngle)) + \
        lightOrbitPosition.y;
      ammonite::models::position::setPosition(lightData[i].linkedModelId,
        glm::vec3(lightPositionX, 5.0f, lightPositionY));
    }

    //Draw the frame
    ammonite::renderer::drawFrame();
    return true;
  }
}
