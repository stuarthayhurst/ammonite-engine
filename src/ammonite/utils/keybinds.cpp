#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <GLFW/glfw3.h>

#include "../core/inputManager.hpp"
#include "../internal/internalSettings.hpp"

#include "../camera.hpp"
#include "../enums.hpp"
#include "timer.hpp"

namespace ammonite {
  namespace utils {
    namespace controls {
      namespace {
        enum DirectionEnum {
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
        int keybindIds[6] = {-1};

        static float* movementSpeedPtr =
          ammonite::settings::controls::internal::getMovementSpeedPtr();
        bool isControlActive = true;
      }

      namespace {
        static void freeCameraCallback(std::vector<int>, int action, void* userPtr) {
          DirectionData* directionData = (DirectionData*)userPtr;

          if (action == GLFW_PRESS) {
            //If it's a key press, start the timer and return
            directionData->directionTimer.reset();
            directionData->directionTimer.unpause();
            return;
          }

          //Get active camera
          int activeCameraId = ammonite::camera::getActiveCamera();

          //Vector for current direction, without vertical component
          float horizontalAngle = ammonite::camera::getHorizontal(activeCameraId);
          glm::vec3 horizontalDirection(std::sin(horizontalAngle),
                              0, std::cos(horizontalAngle));

          //Right vector, relative to the camera
          glm::vec3 right = glm::vec3(std::sin(horizontalAngle - glm::half_pi<float>()),
                                      0, std::cos(horizontalAngle - glm::half_pi<float>()));

          //Get the current camera position
          glm::vec3 position = ammonite::camera::getPosition(activeCameraId);

          //Calculate the new position
          float unitDelta = directionData->directionTimer.getTime() * *movementSpeedPtr;
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
          if (isControlActive) {
            ammonite::camera::setPosition(activeCameraId, position);
          }

          //Reset time between inputs
          if (action == GLFW_RELEASE) {
            //If it's a key release, stop the timer
            directionData->directionTimer.pause();
          }
          directionData->directionTimer.reset();
        }
      }

      void setupFreeCamera(int forwardKey, int backKey, int upKey,
                           int downKey, int rightKey, int leftKey) {
        directionData[0].directionEnum = AMMONITE_FORWARD;
        directionData[1].directionEnum = AMMONITE_BACK;
        directionData[2].directionEnum = AMMONITE_UP;
        directionData[3].directionEnum = AMMONITE_DOWN;
        directionData[4].directionEnum = AMMONITE_RIGHT;
        directionData[5].directionEnum = AMMONITE_LEFT;

        if (forwardKey != -1) {
          keybindIds[0] = ammonite::input::internal::registerRawKeybind(&forwardKey, 1,
            AMMONITE_FORCE_RELEASE, false, freeCameraCallback, &directionData[0]);
        }
        if (backKey != -1) {
          keybindIds[1] = ammonite::input::internal::registerRawKeybind(&backKey, 1,
            AMMONITE_FORCE_RELEASE, false, freeCameraCallback, &directionData[1]);
        }
        if (upKey != -1) {
          keybindIds[2] = ammonite::input::internal::registerRawKeybind(&upKey, 1,
            AMMONITE_FORCE_RELEASE, false, freeCameraCallback, &directionData[2]);
        }
        if (downKey != -1) {
          keybindIds[3] = ammonite::input::internal::registerRawKeybind(&downKey, 1,
            AMMONITE_FORCE_RELEASE, false, freeCameraCallback, &directionData[3]);
        }
        if (rightKey != -1) {
          keybindIds[4] = ammonite::input::internal::registerRawKeybind(&rightKey, 1,
            AMMONITE_FORCE_RELEASE, false, freeCameraCallback, &directionData[4]);
        }
        if (leftKey != -1) {
          keybindIds[5] = ammonite::input::internal::registerRawKeybind(&leftKey, 1,
            AMMONITE_FORCE_RELEASE, false, freeCameraCallback, &directionData[5]);
        }
      }

      void releaseFreeCamera() {
        if (keybindIds[0] != -1) {
          ammonite::input::internal::unregisterKeybind(keybindIds[0]);
          keybindIds[0] = -1;
        }
        if (keybindIds[1] != -1) {
          ammonite::input::internal::unregisterKeybind(keybindIds[1]);
          keybindIds[1] = -1;
        }
        if (keybindIds[2] != -1) {
          ammonite::input::internal::unregisterKeybind(keybindIds[2]);
          keybindIds[2] = -1;
        }
        if (keybindIds[3] != -1) {
          ammonite::input::internal::unregisterKeybind(keybindIds[3]);
          keybindIds[3] = -1;
        }
        if (keybindIds[4] != -1) {
          ammonite::input::internal::unregisterKeybind(keybindIds[4]);
          keybindIds[4] = -1;
        }
        if (keybindIds[5] != -1) {
          ammonite::input::internal::unregisterKeybind(keybindIds[5]);
          keybindIds[5] = -1;
        }
      }

      void setControlsActive(bool active) {
        isControlActive = active;
      }

      bool getControlsActive() {
        return isControlActive;
      }
    }
  }
}
