#include <algorithm>
#include <vector>

#include "controls.hpp"

#include "input.hpp"
#include "keycodes.hpp"

#include "../camera/camera.hpp"
#include "../engine.hpp"
#include "../maths/angle.hpp"
#include "../maths/vector.hpp"
#include "../utils/id.hpp"

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
        float fovLimit = ammonite::twoPi<float> / 3.0f;

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

      //Keyboards controls setup
      DirectionEnum directions[6] = {
        directions[0] = DIRECTION_FORWARD,
        directions[1] = DIRECTION_BACK,
        directions[2] = DIRECTION_UP,
        directions[3] = DIRECTION_DOWN,
        directions[4] = DIRECTION_RIGHT,
        directions[5] = DIRECTION_LEFT
      };

      AmmoniteId keybindIds[6] = {0};

      bool isCameraActive = true;
      bool isZoomActive = true;
    }

    //Keyboard control callbacks
    namespace {
      void keyboardCameraCallback(const std::vector<AmmoniteKeycode>&,
                                  KeyStateEnum action, void* userPtr) {
        DirectionEnum* const directionEnum = (DirectionEnum*)userPtr;

        //Do nothing if the button was released
        if (action == AMMONITE_RELEASED) {
          return;
        }

        //Account for frame rate in movement distance
        const float frameTimeDelta = (float)ammonite::getFrameTime();
        const float unitDelta = frameTimeDelta * controlSettings.movementSpeed;

        //Get active camera, skip if the camera is inactive
        const AmmoniteId activeCameraId = ammonite::camera::getActiveCamera();
        if (!isCameraActive) {
          return;
        }

        //Vector for current direction, without vertical component
        const float horizontalAngle = (float)ammonite::camera::getHorizontal(activeCameraId);
        ammonite::Vec<float, 3> horizontalDirection = {0};
        ammonite::calculateDirection(horizontalAngle, horizontalDirection);

        //Right vector, relative to the camera
        const float angleRight = horizontalAngle - ammonite::halfPi<float>;
        ammonite::Vec<float, 3> right = {0};
        ammonite::calculateDirection(angleRight, right);

        //Up vector, relative to the world
        const ammonite::Vec<float, 3> worldUp = {0.0f, 1.0f, 0.0f};

        //Get the current camera position
        ammonite::Vec<float, 3> position = {0};
        ammonite::camera::getPosition(activeCameraId, position);

        //Determine movement direction
        ammonite::Vec<float, 3> movementDirection = {0};
        switch (*directionEnum) {
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
        if (isZoomActive) {
          if (button == AMMONITE_MOUSE_BUTTON_MIDDLE && action == AMMONITE_PRESSED) {
            ammonite::camera::setFieldOfView(ammonite::camera::getActiveCamera(),
                                             ammonite::pi<float> / 4.0f);
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

    //Enable / disable camera position, direction and zoom
    void setCameraActive(bool active) {
      isCameraActive = active;
      isZoomActive = active;
    }

    /*
     - Enable / disable camera position, direction and zoom
     - Give separate control over zoom
    */
    void setCameraActive(bool active, bool allowZoom) {
      isCameraActive = active;
      isZoomActive = allowZoom;
    }

    bool getCameraActive() {
      return isCameraActive;
    }

    bool getZoomActive() {
      return isZoomActive;
    }

    void setupFreeCamera(AmmoniteKeycode forwardKey, AmmoniteKeycode backKey,
                         AmmoniteKeycode upKey, AmmoniteKeycode downKey,
                         AmmoniteKeycode rightKey, AmmoniteKeycode leftKey) {
      //Set keyboard callbacks
      const AmmoniteKeycode keycodes[6] = {forwardKey, backKey, upKey, downKey, rightKey, leftKey};
      for (int i = 0; i < 6; i++) {
        if (keycodes[i] != 0) {
          keybindIds[i] = ammonite::input::internal::registerRawKeybind(&keycodes[i], 1,
            AMMONITE_FORCE_RELEASE, false, keyboardCameraCallback, &directions[i]);
        }
      }

      //Set mouse control callbacks
      ammonite::input::setCursorPositionCallback(cursorPositionCallback, nullptr);
      ammonite::input::setMouseButtonCallback(zoomResetCallback, nullptr);
      ammonite::input::setScrollWheelCallback(scrollCallback, nullptr);
    }

    void releaseFreeCamera() {
      //Clean up keybinds
      for (AmmoniteId& keybindId : keybindIds) {
        if (keybindId != 0) {
          ammonite::input::internal::unregisterKeybind(keybindId);
          keybindId = 0;
        }
      }

      //Mouse callback clean up
      ammonite::input::setCursorPositionCallback(nullptr, nullptr);
      ammonite::input::setMouseButtonCallback(nullptr, nullptr);
      ammonite::input::setScrollWheelCallback(nullptr, nullptr);
    }
  }
}
