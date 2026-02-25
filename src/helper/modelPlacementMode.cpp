#include <algorithm>
#include <iostream>
#include <vector>

#include <ammonite/ammonite.hpp>

#include "modelPlacementMode.hpp"

#include "torusGenerator.hpp"

namespace placement {
  namespace {
    //Currently placing model data
    bool modelPlacementModeEnabled = false;
    float modelDistance = 0.0f;
    AmmoniteId placementModelId = 0;

    AmmoniteId placementModeKeybindId;
    std::vector<AmmoniteId> placedModelIds;
  }

  namespace {
    AmmoniteId createTorus() {
      //Calculate mesh size
      const unsigned int torusWidth = 100;
      const unsigned int torusHeight = 100;
      const unsigned int torusVertexCount = torus::getVertexCount(torusWidth, torusHeight);
      const unsigned int torusIndexCount = torus::getIndexCount(torusWidth, torusHeight);

      //Calculate size parameters
      const float volumeDiameter = 0.55f;
      const float ringRadius = torus::calculateMaxRingRadius(volumeDiameter);

      //Generate the torus
      ammonite::models::AmmoniteVertex* meshData = nullptr;
      unsigned int* indexData = nullptr;
      torus::generateTorus(ringRadius, volumeDiameter, torusWidth, torusHeight,
                           &meshData, &indexData);

      //Material settings
      const ammonite::models::AmmoniteMaterial material =
        ammonite::models::createMaterial({0.1f, 1.0f, 0.5f}, {0.5f, 0.5f, 0.5f});

      //Upload the torus
      const unsigned int torusId = ammonite::models::createModel(&meshData[0],
        &indexData[0], material, torusVertexCount, torusIndexCount);
      ammonite::models::deleteMaterial(material);

      //Clean up and return
      delete [] meshData;
      delete [] indexData;
      return torusId;
    }
  }

  //Input callbacks
  namespace {
    void mouseButtonCallback(AmmoniteButton button, KeyStateEnum action, void*) {
      //Disable placement on left-click
      if (modelPlacementModeEnabled) {
        if (button == AMMONITE_MOUSE_BUTTON_LEFT) {
          modelPlacementModeEnabled = false;
          placementModelId = 0;
          resetPlacementDistance();

          return;
        }
      }

      //Handle zoom reset logic
      if (ammonite::controls::getZoomActive()) {
        if (button == AMMONITE_MOUSE_BUTTON_MIDDLE && action == AMMONITE_PRESSED) {
          if (modelPlacementModeEnabled) {
            resetPlacementDistance();
          } else {
            ammonite::camera::setFieldOfView(ammonite::camera::getActiveCamera(),
                                             ammonite::pi<float> / 4.0f);
          }
        }
      }
    }

    void scrollCallback(double, double yOffset, void*) {
      if (modelPlacementModeEnabled) {
        const float zoomSpeed = ammonite::controls::settings::getRealZoomSpeed();
        const float newModelDistance = modelDistance + ((float)yOffset * zoomSpeed * 4.0f);
        modelDistance = std::max(newModelDistance, 1.0f);

        return;
      }

      //Handle usual zoom logic
      if (ammonite::controls::getZoomActive()) {
        const AmmoniteId activeCameraId = ammonite::camera::getActiveCamera();
        const float fov = ammonite::camera::getFieldOfView(activeCameraId);

        //Only zoom if FoV will be between 0.1 and FoV limit
        const float zoomSpeed = ammonite::controls::settings::getRealZoomSpeed();
        const float newFov = fov - ((float)yOffset * zoomSpeed);
        ammonite::camera::setFieldOfView(activeCameraId,
          std::clamp(newFov, 0.1f, ammonite::controls::settings::getFovLimit()));
      }
    }

    void placementCallback(const std::vector<AmmoniteKeycode>&, KeyStateEnum, void*) {
      //Delete the model being placed and return if it's already active
      if (modelPlacementModeEnabled) {
        ammonite::models::deleteModel(placementModelId);
        placedModelIds.erase(std::ranges::find(placedModelIds, placementModelId));

        ammonite::utils::status << "Destroyed object" << std::endl;
        modelPlacementModeEnabled = false;
        return;
      }

      //Copy or load a torus
      if (!placedModelIds.empty()) {
        placementModelId = ammonite::models::copyModel(placedModelIds[0], true);
      } else {
        placementModelId = createTorus();
      }
      placedModelIds.push_back(placementModelId);

      //Enter placement mode
      modelPlacementModeEnabled = true;
      resetPlacementDistance();

      ammonite::utils::status << "Spawned object" << std::endl;
    }
  }

  //Setup mouse callbacks and keybinds for model placement mode
  void setPlacementCallbacks() {
    ammonite::input::setMouseButtonCallback(mouseButtonCallback, nullptr);
    ammonite::input::setScrollWheelCallback(scrollCallback, nullptr);

    placementModeKeybindId = ammonite::input::registerToggleKeybind(AMMONITE_P,
      placementCallback, nullptr);
  }

  void unsetPlacementCallbacks() {
    if (placementModeKeybindId != 0) {
      ammonite::input::unregisterKeybind(placementModeKeybindId);

      ammonite::input::setMouseButtonCallback(nullptr, nullptr);
      ammonite::input::setScrollWheelCallback(nullptr, nullptr);
    }
  }

  void resetPlacementDistance() {
    modelDistance = 3.0f;
  }

  void updatePlacementPosition() {
    if (!modelPlacementModeEnabled) {
      return;
    }

    //Fetch camera data
    const AmmoniteId activeCameraId = ammonite::camera::getActiveCamera();
    ammonite::Vec<float, 3> cameraPosition = {0};
    ammonite::Vec<float, 3> cameraDirection = {0};
    ammonite::camera::getPosition(activeCameraId, cameraPosition);
    ammonite::camera::getDirection(activeCameraId, cameraDirection);
    const double horiz = ammonite::camera::getHorizontal(activeCameraId);
    const double vert = ammonite::camera::getVertical(activeCameraId);

    //Calculate position
    ammonite::Vec<float, 3> modelPosition = {0};
    ammonite::scale(cameraDirection, modelDistance, modelPosition);
    ammonite::add(modelPosition, cameraPosition);

    //Place the model
    const ammonite::Vec<float, 3> modelRotation = {-(float)vert, (float)horiz, 0.0f};
    ammonite::models::position::setRotation(placementModelId, modelRotation);
    ammonite::models::position::setScale(placementModelId, 0.25f);
    ammonite::models::position::setPosition(placementModelId, modelPosition);
  }

  void deletePlacedModels() {
    for (const AmmoniteId& modelId : placedModelIds) {
      ammonite::models::deleteModel(modelId);
    }
  }
}
