#include <cmath>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

extern "C" {
  #include <GLFW/glfw3.h>
}

#include "controls.hpp"

#include "input.hpp"

#include "../camera.hpp"
#include "../utils/id.hpp"
#include "../utils/timer.hpp"

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
        AMMONITE_FORWARD,
        AMMONITE_BACK,
        AMMONITE_UP,
        AMMONITE_DOWN,
        AMMONITE_RIGHT,
        AMMONITE_LEFT
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
      void keyboardCameraCallback(const std::vector<int>&, KeyStateEnum action, void* userPtr) {
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
        const float horizontalAngle = ammonite::camera::getHorizontal(activeCameraId);
        const glm::vec3 horizontalDirection(std::sin(horizontalAngle), 0,
                                            std::cos(horizontalAngle));

        //Right vector, relative to the camera
        const glm::vec3 right = glm::vec3(std::sin(horizontalAngle - glm::half_pi<float>()), 0,
                                          std::cos(horizontalAngle - glm::half_pi<float>()));

        //Get the current camera position
        glm::vec3 position = ammonite::camera::getPosition(activeCameraId);

        //Calculate the new position
        const float unitDelta = (float)directionTimer->getTime() * controlSettings.movementSpeed;
        switch (directionData->directionEnum) {
        case AMMONITE_FORWARD:
          position += horizontalDirection * unitDelta;
          break;
        case AMMONITE_BACK:
          position -= horizontalDirection * unitDelta;
          break;
        case AMMONITE_UP:
          position += glm::vec3(0, 1, 0) * unitDelta;
          break;
        case AMMONITE_DOWN:
          position -= glm::vec3(0, 1, 0) * unitDelta;
          break;
        case AMMONITE_RIGHT:
          position += right * unitDelta;
          break;
        case AMMONITE_LEFT:
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
      void zoomResetCallback(int button, KeyStateEnum action, void*) {
        if (isCameraActive) {
          if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == AMMONITE_PRESSED) {
            ammonite::camera::setFieldOfView(ammonite::camera::getActiveCamera(),
                                             glm::quarter_pi<float>());
          }
        }
      }

      void cursorPositionCallback(double, double, double xOffset, double yOffset, void*) {
        if (isCameraActive) {
          //Get current viewing angles
          const AmmoniteId activeCameraId = ammonite::camera::getActiveCamera();
          const float horizontalAngle = ammonite::camera::getHorizontal(activeCameraId);
          float verticalAngle = ammonite::camera::getVertical(activeCameraId);

          //Update viewing angles ('-' corrects camera inversion)
          ammonite::camera::setHorizontal(activeCameraId,
            horizontalAngle - (controlSettings.mouseSpeed * (float)xOffset));

          //Only accept vertical angle if it won't create an impossible movement
          const float newAngle = verticalAngle - (controlSettings.mouseSpeed * (float)yOffset);
          static const float limit = glm::radians(90.0f);
          if (newAngle > limit) { //Vertical max
            verticalAngle = limit;
          } else if (newAngle < -limit) { //Vertical min
            verticalAngle = -limit;
          } else {
            verticalAngle += -controlSettings.mouseSpeed * (float)yOffset;
          }
          ammonite::camera::setVertical(activeCameraId, verticalAngle);
        }
      }
    }

    void setCameraActive(bool active) {
      isCameraActive = active;
    }

    bool getCameraActive() {
      return isCameraActive;
    }

    void setupFreeCamera(int forwardKey, int backKey, int upKey,
                         int downKey, int rightKey, int leftKey) {
      //Keyboards controls setup
      directionData[0].directionEnum = AMMONITE_FORWARD;
      directionData[1].directionEnum = AMMONITE_BACK;
      directionData[2].directionEnum = AMMONITE_UP;
      directionData[3].directionEnum = AMMONITE_DOWN;
      directionData[4].directionEnum = AMMONITE_RIGHT;
      directionData[5].directionEnum = AMMONITE_LEFT;

      //Set keyboard callbacks
      const int keycodes[6] = {forwardKey, backKey, upKey, downKey, rightKey, leftKey};
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
