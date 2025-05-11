#include <cmath>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "controls.hpp"

#include "input.hpp"
#include "keycodes.hpp"

#include "../camera.hpp"
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
        float fovLimit = glm::two_pi<float>() / 3;

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
        DirectionData* directionData = (DirectionData*)userPtr;
        ammonite::utils::Timer* directionTimer = &directionData->directionTimer;
        //If it's an initial key press, start the timer and return
        if (action == AMMONITE_PRESSED) {
          directionTimer->reset();
          directionTimer->unpause();
          return;
        }

        //Get active camera
        const AmmoniteId activeCameraId = ammonite::camera::getActiveCamera();

        //Vector for current direction, without vertical component
        const double horizontalAngle = ammonite::camera::getHorizontal(activeCameraId);
        const glm::vec3 horizontalDirection(std::sin(horizontalAngle), 0,
                                            std::cos(horizontalAngle));

        //Right vector, relative to the camera
        const glm::vec3 right = glm::vec3(std::sin(horizontalAngle - glm::half_pi<double>()), 0,
                                          std::cos(horizontalAngle - glm::half_pi<double>()));

        //Get the current camera position
        glm::vec3 position = ammonite::camera::getPosition(activeCameraId);

        //Calculate the new position
        const float unitDelta = (float)directionTimer->getTime() * controlSettings.movementSpeed;
        switch (directionData->directionEnum) {
        case DIRECTION_FORWARD:
          position += horizontalDirection * unitDelta;
          break;
        case DIRECTION_BACK:
          position -= horizontalDirection * unitDelta;
          break;
        case DIRECTION_UP:
          position += glm::vec3(0, 1, 0) * unitDelta;
          break;
        case DIRECTION_DOWN:
          position -= glm::vec3(0, 1, 0) * unitDelta;
          break;
        case DIRECTION_RIGHT:
          position += right * unitDelta;
          break;
        case DIRECTION_LEFT:
          position -= right * unitDelta;
          break;
        }

        //Update the camera position
        if (isCameraActive) {
          ammonite::camera::setPosition(activeCameraId, position);
        }

        //Reset time between inputs
        if (action == AMMONITE_RELEASED) {
          //If it's a key release, stop the timer
          directionTimer->pause();
        }
        directionTimer->reset();
      }
    }

    //Mouse control callbacks
    namespace {
      //Increase / decrease field of view on scroll
      void scrollCallback(double, double yOffset, void*) {
        if (isCameraActive) {
          const AmmoniteId activeCameraId = ammonite::camera::getActiveCamera();
          const float fov = ammonite::camera::getFieldOfView(activeCameraId);

          //Only zoom if FoV will be between 1 and FoV limit
          const float newFov = fov - ((float)yOffset * controlSettings.zoomSpeed);
          if (newFov > 0 && newFov <= controlSettings.fovLimit) {
            ammonite::camera::setFieldOfView(activeCameraId, newFov);
          }
        }
      }

      //Reset field of view on middle click, (modifier bits are unused)
      void zoomResetCallback(AmmoniteButton button, KeyStateEnum action, void*) {
        if (isCameraActive) {
          if (button == AMMONITE_MOUSE_BUTTON_MIDDLE && action == AMMONITE_PRESSED) {
            ammonite::camera::setFieldOfView(ammonite::camera::getActiveCamera(),
                                             glm::quarter_pi<float>());
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

          //Only accept vertical angle if it won't create an impossible movement
          double newVerticalAngle = verticalAngle - (controlSettings.mouseSpeed * yOffset);
          const double limit = glm::radians(90.0);
          if (newVerticalAngle > limit) { //Vertical max
            newVerticalAngle = limit;
          } else if (newVerticalAngle < -limit) { //Vertical min
            newVerticalAngle = -limit;
          }

          //Update the camera
          ammonite::camera::setAngle(activeCameraId, horizontalAngle, newVerticalAngle);
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
          keybindIds[0] = ammonite::input::internal::registerRawKeybind(&keycodes[i], 1,
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
