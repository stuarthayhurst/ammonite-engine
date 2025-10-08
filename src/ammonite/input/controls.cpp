#include <algorithm>
#include <cmath>
#include <vector>

#include "controls.hpp"

#include "input.hpp"
#include "keycodes.hpp"

#include "../camera.hpp"
#include "../maths/angle.hpp"
#include "../maths/vector.hpp"
#include "../utils/id.hpp"
#include "../utils/timer.hpp"

/*
 - This file provides some ready-made camera controls, built on the input layer
 - It shouldn't rely on engine internals or engine dependencies to work
*/

//Store and expose controls settings
namespace ammonite {
  namespace controls {
    namespace {
      //Structure to store base speeds, multipliers and calculated speeds
      struct ControlSettings {
        struct BaseControlSettings {
          float movementSpeed = 5.0f;
          float mouseSpeed = 0.005f;
          float zoomSpeed = 0.025f;
        } baseSettings;

        struct ControlMultipliers {
          float movement = 1.0f;
          float mouse = 1.0f;
          float zoom = 1.0f;
        } multipliers;

        //Default to a 120 degree field of view limit
        float fovLimit = (2.0f * ammonite::pi<float>()) / 3.0f;

        //Final sensitivities
        float movementSpeed = baseSettings.movementSpeed;
        float mouseSpeed = baseSettings.mouseSpeed;
        float zoomSpeed = baseSettings.zoomSpeed;
      } controlSettings;
    }

    namespace settings {
      void setMovementSpeed(float newMovementSpeed) {
        controlSettings.multipliers.movement = newMovementSpeed;
        controlSettings.movementSpeed = controlSettings.baseSettings.movementSpeed *
          newMovementSpeed;
      }

      void setMouseSpeed(float newMouseSpeed) {
        controlSettings.multipliers.mouse = newMouseSpeed;
        controlSettings.mouseSpeed = controlSettings.baseSettings.mouseSpeed * newMouseSpeed;
      }

      void setZoomSpeed(float newZoomSpeed) {
        controlSettings.multipliers.zoom = newZoomSpeed;
        controlSettings.zoomSpeed = controlSettings.baseSettings.zoomSpeed * newZoomSpeed;
      }

      //Radians
      void setFovLimit(float newFovLimit) {
        controlSettings.fovLimit = newFovLimit;
      }

      float getMovementSpeed() {
        return controlSettings.multipliers.movement;
      }

      float getMouseSpeed() {
        return controlSettings.multipliers.mouse;
      }

      float getZoomSpeed() {
        return controlSettings.multipliers.zoom;
      }

      //Radians
      float getFovLimit() {
        return controlSettings.fovLimit;
      }

      float getRealMovementSpeed() {
        return controlSettings.movementSpeed;
      }

      float getRealMouseSpeed() {
        return controlSettings.mouseSpeed;
      }

      float getRealZoomSpeed() {
        return controlSettings.zoomSpeed;
      }
    }
  }
}

namespace ammonite {
  namespace controls {
    namespace {
      //Keyboard control direction enums
      enum DirectionEnum : unsigned char {
        DIRECTION_FORWARD,
        DIRECTION_BACK,
        DIRECTION_UP,
        DIRECTION_DOWN,
        DIRECTION_RIGHT,
        DIRECTION_LEFT
      };

      //Data to handle independent camera directions
      struct DirectionData {
        ammonite::utils::Timer directionTimer;
        DirectionEnum directionEnum;
      } directionData[6];
      AmmoniteId keybindIds[6] = {0};

      bool isCameraActive = true;
    }

    //Keyboard control callbacks
    namespace {
      void keyboardCameraCallback(const std::vector<AmmoniteKeycode>&,
                                  KeyStateEnum action, void* userPtr) {
        DirectionData* const directionData = (DirectionData*)userPtr;
        ammonite::utils::Timer* directionTimer = &directionData->directionTimer;
        //If it's an initial key press, start the timer and return
        if (action == AMMONITE_PRESSED) {
          directionTimer->reset();
          directionTimer->unpause();
          return;
        }

        //Get active camera
        const AmmoniteId activeCameraId = ammonite::camera::getActiveCamera();

        //Reset time between inputs
        const float unitDelta = (float)directionTimer->getTime() * controlSettings.movementSpeed;
        if (action == AMMONITE_RELEASED) {
          //If it's a key release, stop the timer
          directionTimer->pause();
        }
        directionTimer->reset();

        //Skip if the camera is inactive
        if (!isCameraActive) {
          return;
        }

        //Vector for current direction, without vertical component
        const float horizontalAngle = (float)ammonite::camera::getHorizontal(activeCameraId);
        const ammonite::Vec<float, 3> horizontalDirection = {
          std::sin(horizontalAngle), 0.0f, std::cos(horizontalAngle)
        };

        //Right vector, relative to the camera
        const float angleRight = horizontalAngle - (ammonite::pi<float>() / 2.0f);
        const ammonite::Vec<float, 3> right = {
          std::sin(angleRight), 0.0f, std::cos(angleRight)
        };

        //Up vector, relative to the world
        const ammonite::Vec<float, 3> worldUp = {0.0f, 1.0f, 0.0f};

        //Get the current camera position
        ammonite::Vec<float, 3> position = {0};
        ammonite::camera::getPosition(activeCameraId, position);

        //Determine movement direction
        ammonite::Vec<float, 3> movementDirection = {0};
        switch (directionData->directionEnum) {
        case DIRECTION_FORWARD:
          ammonite::copy(horizontalDirection, movementDirection);
          break;
        case DIRECTION_BACK:
          ammonite::negate(horizontalDirection, movementDirection);
          break;
        case DIRECTION_UP:
          ammonite::copy(worldUp, movementDirection);
          break;
        case DIRECTION_DOWN:
          ammonite::negate(worldUp, movementDirection);
          break;
        case DIRECTION_RIGHT:
          ammonite::copy(right, movementDirection);
          break;
        case DIRECTION_LEFT:
          ammonite::negate(right, movementDirection);
          break;
        }

        //Calculate new position
        ammonite::Vec<float, 3> positionDelta = {0};
        ammonite::Vec<float, 3> newPosition = {0};
        ammonite::scale(movementDirection, unitDelta, positionDelta);
        ammonite::add(position, positionDelta, newPosition);

        //Update the camera position
        ammonite::camera::setPosition(activeCameraId, newPosition);
      }
    }

    //Mouse control callbacks
    namespace {
      //Increase / decrease field of view on scroll
      void scrollCallback(double, double yOffset, void*) {
        if (isCameraActive) {
          const AmmoniteId activeCameraId = ammonite::camera::getActiveCamera();
          const float fov = ammonite::camera::getFieldOfView(activeCameraId);

          //Only zoom if FoV will be between 0.1 and FoV limit
          const float newFov = fov - ((float)yOffset * controlSettings.zoomSpeed);
          ammonite::camera::setFieldOfView(activeCameraId,
            std::clamp(newFov, 0.1f, controlSettings.fovLimit));
        }
      }

      //Reset field of view on middle click
      void zoomResetCallback(AmmoniteButton button, KeyStateEnum action, void*) {
        if (isCameraActive) {
          if (button == AMMONITE_MOUSE_BUTTON_MIDDLE && action == AMMONITE_PRESSED) {
            ammonite::camera::setFieldOfView(ammonite::camera::getActiveCamera(),
                                             ammonite::pi<float>() / 4.0f);
          }
        }
      }

      void cursorPositionCallback(double, double, double xOffset, double yOffset, void*) {
        if (isCameraActive) {
          //Get current viewing angles
          const AmmoniteId activeCameraId = ammonite::camera::getActiveCamera();
          double horizontalAngle = ammonite::camera::getHorizontal(activeCameraId);
          const double verticalAngle = ammonite::camera::getVertical(activeCameraId);

          //Calculate new horizontal angle
          horizontalAngle -= controlSettings.mouseSpeed * xOffset;

          //Update the camera, restricting vertical movement
          const double newVerticalAngle = verticalAngle - (controlSettings.mouseSpeed * yOffset);
          const double limit = ammonite::radians(90.0);
          ammonite::camera::setAngle(activeCameraId, horizontalAngle,
                                     std::clamp(newVerticalAngle, -limit, limit));
        }
      }
    }

    void setCameraActive(bool active) {
      isCameraActive = active;
    }

    bool getCameraActive() {
      return isCameraActive;
    }

    void setupFreeCamera(AmmoniteKeycode forwardKey, AmmoniteKeycode backKey,
                         AmmoniteKeycode upKey, AmmoniteKeycode downKey,
                         AmmoniteKeycode rightKey, AmmoniteKeycode leftKey) {
      //Keyboards controls setup
      directionData[0].directionEnum = DIRECTION_FORWARD;
      directionData[1].directionEnum = DIRECTION_BACK;
      directionData[2].directionEnum = DIRECTION_UP;
      directionData[3].directionEnum = DIRECTION_DOWN;
      directionData[4].directionEnum = DIRECTION_RIGHT;
      directionData[5].directionEnum = DIRECTION_LEFT;

      //Set keyboard callbacks
      const AmmoniteKeycode keycodes[6] = {forwardKey, backKey, upKey, downKey, rightKey, leftKey};
      for (int i = 0; i < 6; i++) {
        if (keycodes[i] != 0) {
          keybindIds[i] = ammonite::input::internal::registerRawKeybind(&keycodes[i], 1,
            AMMONITE_FORCE_RELEASE, false, keyboardCameraCallback, &directionData[i]);
        }
      }

      //Set mouse control callbacks
      ammonite::input::setCursorPositionCallback(cursorPositionCallback, nullptr);
      ammonite::input::setMouseButtonCallback(zoomResetCallback, nullptr);
      ammonite::input::setScrollWheelCallback(scrollCallback, nullptr);
    }

    void releaseFreeCamera() {
      //Clean up keybinds
      for (int i = 0; i < 6; i++) {
        if (keybindIds[i] != 0) {
          ammonite::input::internal::unregisterKeybind(keybindIds[i]);
          keybindIds[i] = 0;
        }
      }

      //Mouse callback clean up
      ammonite::input::setCursorPositionCallback(nullptr, nullptr);
      ammonite::input::setMouseButtonCallback(nullptr, nullptr);
      ammonite::input::setScrollWheelCallback(nullptr, nullptr);
    }
  }
}
