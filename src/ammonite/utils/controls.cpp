#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <GLFW/glfw3.h>

#include "../core/inputManager.hpp"
#include "../core/windowManager.hpp"
#include "../internal/internalSettings.hpp"

#include "../camera.hpp"
#include "../enums.hpp"
#include "timer.hpp"

namespace ammonite {
  namespace utils {
    namespace {
      GLFWwindow* windowPtr = nullptr;
      typedef void (*CursorPositionCallback)(GLFWwindow*, double, double);
      CursorPositionCallback activeCursorPositionCallback = nullptr;

      //Keyboard control direction enums
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

      //Various pointers to other systems, used by callbacks
      bool* isInputBlockedPtr = ammonite::input::internal::getInputBlockPtr();
      static float* movementSpeedPtr =
        ammonite::settings::controls::internal::getMovementSpeedPtr();
      float* zoomSpeedPtr = ammonite::settings::controls::internal::getZoomSpeedPtr();
      float* fovLimitPtr = ammonite::settings::controls::internal::getFovLimitPtr();
      float* mouseSpeedPtr = ammonite::settings::controls::internal::getMouseSpeedPtr();

      //Mouse position callback data
      double xposLast, yposLast;
      bool ignoreNextCursor = false;

      bool isCameraActive = true;
    }

    //Keyboard control callbacks
    namespace {
      static void keyboardCameraCallback(std::vector<int>, int action, void* userPtr) {
        DirectionData* directionData = (DirectionData*)userPtr;
        //If it's an initial key press, start the timer and return
        if (action == GLFW_PRESS) {
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
        if (isCameraActive) {
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

    //Mouse control callbacks
    namespace {
      //Increase / decrease FoV on scroll (xoffset is unused)
      static void scrollCallback(GLFWwindow*, double, double yoffset) {
        if (!(*isInputBlockedPtr) and isCameraActive) {
          int activeCameraId = ammonite::camera::getActiveCamera();
          float fov = ammonite::camera::getFieldOfView(activeCameraId);

          //Only zoom if FoV will be between 1 and FoV limit
          float newFov = fov - (yoffset * *zoomSpeedPtr);
          if (newFov > 0 and newFov <= *fovLimitPtr) {
            ammonite::camera::setFieldOfView(activeCameraId, newFov);
          }
        }
      }

      //Reset FoV on middle click, (modifier bits are unused)
      static void zoomResetCallback(GLFWwindow*, int button, int action, int) {
        if (!(*isInputBlockedPtr) and isCameraActive) {
          if (button == GLFW_MOUSE_BUTTON_MIDDLE and action == GLFW_PRESS) {
            ammonite::camera::setFieldOfView(ammonite::camera::getActiveCamera(),
                                             glm::quarter_pi<float>());
          }
        }
      }

      static void cursorPositionCallback(GLFWwindow*, double xpos, double ypos) {
        if (isCameraActive) {
          //Work out distance moved since last movement
          float xoffset = xpos - xposLast;
          float yoffset = ypos - yposLast;

          //Update saved cursor positions
          xposLast = xpos;
          yposLast = ypos;

          if (ignoreNextCursor) {
            ignoreNextCursor = false;
            return;
          }

          //Get current viewing angles
          int activeCameraId = ammonite::camera::getActiveCamera();
          float horizontalAngle = ammonite::camera::getHorizontal(activeCameraId);
          float verticalAngle = ammonite::camera::getVertical(activeCameraId);

          //Update viewing angles ('-' corrects camera inversion)
          ammonite::camera::setHorizontal(activeCameraId,
                                          horizontalAngle - (*mouseSpeedPtr * xoffset));

          //Only accept vertical angle if it won't create an impossible movement
          float newAngle = verticalAngle - (*mouseSpeedPtr * yoffset);
          static const float limit = glm::radians(90.0f);
          if (newAngle > limit) { //Vertical max
            verticalAngle = limit;
          } else if (newAngle < -limit) { //Vertical min
            verticalAngle = -limit;
          } else {
            verticalAngle += -*mouseSpeedPtr * yoffset;
          }
          ammonite::camera::setVertical(activeCameraId, verticalAngle);
        }
      }
    }

    //Internally exposed function to set cursor focus from input system
    namespace controls {
      namespace internal {
        //Helper function to set input state
        void setCursorFocus(bool inputFocused) {
          //Skip next cursor movement, to avoid huge jumps
          ignoreNextCursor = true;

          if (windowPtr == nullptr) {
            return;
          }

          //Hide and unhide cursor as necessary
          if (inputFocused) {
            //Hide cursor and start taking mouse input
            glfwSetInputMode(windowPtr, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwSetCursorPosCallback(windowPtr, activeCursorPositionCallback);
            //Reset saved cursor position to avoid a large jump
            glfwGetCursorPos(windowPtr, &xposLast, &yposLast);
          } else {
            //Remove callback and restore cursor
            glfwSetCursorPosCallback(windowPtr, nullptr);
            glfwSetInputMode(windowPtr, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
          }
        }
      }
    }

    namespace controls {
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
        if (forwardKey != -1) {
          keybindIds[0] = ammonite::input::internal::registerRawKeybind(&forwardKey, 1,
            AMMONITE_FORCE_RELEASE, false, keyboardCameraCallback, &directionData[0]);
        }
        if (backKey != -1) {
          keybindIds[1] = ammonite::input::internal::registerRawKeybind(&backKey, 1,
            AMMONITE_FORCE_RELEASE, false, keyboardCameraCallback, &directionData[1]);
        }
        if (upKey != -1) {
          keybindIds[2] = ammonite::input::internal::registerRawKeybind(&upKey, 1,
            AMMONITE_FORCE_RELEASE, false, keyboardCameraCallback, &directionData[2]);
        }
        if (downKey != -1) {
          keybindIds[3] = ammonite::input::internal::registerRawKeybind(&downKey, 1,
            AMMONITE_FORCE_RELEASE, false, keyboardCameraCallback, &directionData[3]);
        }
        if (rightKey != -1) {
          keybindIds[4] = ammonite::input::internal::registerRawKeybind(&rightKey, 1,
            AMMONITE_FORCE_RELEASE, false, keyboardCameraCallback, &directionData[4]);
        }
        if (leftKey != -1) {
          keybindIds[5] = ammonite::input::internal::registerRawKeybind(&leftKey, 1,
            AMMONITE_FORCE_RELEASE, false, keyboardCameraCallback, &directionData[5]);
        }

        //Mouse controls setup, prepare cursor position and mode
        ignoreNextCursor = true;
        windowPtr = ammonite::window::internal::getWindowPtr();
        glfwSetInputMode(windowPtr, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwGetCursorPos(windowPtr, &xposLast, &yposLast);

        //Set mouse control callbacks
        glfwSetScrollCallback(windowPtr, scrollCallback);
        glfwSetMouseButtonCallback(windowPtr, zoomResetCallback);
        glfwSetCursorPosCallback(windowPtr, cursorPositionCallback);
        activeCursorPositionCallback = cursorPositionCallback;
      }

      void releaseFreeCamera() {
        //Clean up keybinds
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

        //Mouse callback clean up
        glfwSetScrollCallback(windowPtr, nullptr);
        glfwSetMouseButtonCallback(windowPtr, nullptr);
        glfwSetCursorPosCallback(windowPtr, nullptr);
        activeCursorPositionCallback = nullptr;

        //Reset input mode and window pointer
        glfwSetInputMode(windowPtr, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        windowPtr = nullptr;
      }
    }
  }
}
