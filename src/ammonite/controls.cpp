#include <cmath>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

extern "C" {
  #include <GLFW/glfw3.h>
}

#include "controls.hpp"

#include "camera.hpp"
#include "enums.hpp"
#include "input.hpp"
#include "types.hpp"
#include "utils/timer.hpp"
#include "window/window.hpp"

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
      GLFWwindow* windowPtr = nullptr;
      using CursorPositionCallback = void (*)(GLFWwindow*, double, double);
      CursorPositionCallback activeCursorPositionCallback = nullptr;

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

      //Various pointers to other systems, used by callbacks
      bool* isInputBlockedPtr = ammonite::input::internal::getInputBlockPtr();

      //Mouse position callback data
      double xposLast, yposLast;
      bool ignoreNextCursor = false;

      bool isCameraActive = true;
    }

    //Keyboard control callbacks
    namespace {
      void keyboardCameraCallback(const std::vector<int>&, int action, void* userPtr) {
        DirectionData* directionData = (DirectionData*)userPtr;
        ammonite::utils::Timer* directionTimer = &directionData->directionTimer;
        //If it's an initial key press, start the timer and return
        if (action == GLFW_PRESS) {
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
        if (action == GLFW_RELEASE) {
          //If it's a key release, stop the timer
          directionTimer->pause();
        }
        directionTimer->reset();
      }
    }

    //Mouse control callbacks
    namespace {
      //Increase / decrease FoV on scroll (xoffset is unused)
      void scrollCallback(GLFWwindow*, double, double yoffset) {
        if (!(*isInputBlockedPtr) && isCameraActive) {
          const AmmoniteId activeCameraId = ammonite::camera::getActiveCamera();
          const float fov = ammonite::camera::getFieldOfView(activeCameraId);

          //Only zoom if FoV will be between 1 and FoV limit
          const float newFov = fov - ((float)yoffset * controlSettings.zoomSpeed);
          if (newFov > 0 && newFov <= controlSettings.fovLimit) {
            ammonite::camera::setFieldOfView(activeCameraId, newFov);
          }
        }
      }

      //Reset FoV on middle click, (modifier bits are unused)
      void zoomResetCallback(GLFWwindow*, int button, int action, int) {
        if (!(*isInputBlockedPtr) && isCameraActive) {
          if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
            ammonite::camera::setFieldOfView(ammonite::camera::getActiveCamera(),
                                             glm::quarter_pi<float>());
          }
        }
      }

      void cursorPositionCallback(GLFWwindow*, double xpos, double ypos) {
        if (isCameraActive) {
          //Work out distance moved since last movement
          const float xoffset = (float)(xpos - xposLast);
          const float yoffset = (float)(ypos - yposLast);

          //Update saved cursor positions
          xposLast = xpos;
          yposLast = ypos;

          if (ignoreNextCursor) {
            ignoreNextCursor = false;
            return;
          }

          //Get current viewing angles
          const AmmoniteId activeCameraId = ammonite::camera::getActiveCamera();
          const float horizontalAngle = ammonite::camera::getHorizontal(activeCameraId);
          float verticalAngle = ammonite::camera::getVertical(activeCameraId);

          //Update viewing angles ('-' corrects camera inversion)
          ammonite::camera::setHorizontal(activeCameraId,
            horizontalAngle - (controlSettings.mouseSpeed * xoffset));

          //Only accept vertical angle if it won't create an impossible movement
          const float newAngle = verticalAngle - (controlSettings.mouseSpeed * yoffset);
          static const float limit = glm::radians(90.0f);
          if (newAngle > limit) { //Vertical max
            verticalAngle = limit;
          } else if (newAngle < -limit) { //Vertical min
            verticalAngle = -limit;
          } else {
            verticalAngle += -controlSettings.mouseSpeed * yoffset;
          }
          ammonite::camera::setVertical(activeCameraId, verticalAngle);
        }
      }
    }

    //Internally exposed function to set cursor focus from input system
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
      if (forwardKey != 0) {
        keybindIds[0] = ammonite::input::internal::registerRawKeybind(&forwardKey, 1,
          AMMONITE_FORCE_RELEASE, false, keyboardCameraCallback, &directionData[0]);
      }
      if (backKey != 0) {
        keybindIds[1] = ammonite::input::internal::registerRawKeybind(&backKey, 1,
          AMMONITE_FORCE_RELEASE, false, keyboardCameraCallback, &directionData[1]);
      }
      if (upKey != 0) {
        keybindIds[2] = ammonite::input::internal::registerRawKeybind(&upKey, 1,
          AMMONITE_FORCE_RELEASE, false, keyboardCameraCallback, &directionData[2]);
      }
      if (downKey != 0) {
        keybindIds[3] = ammonite::input::internal::registerRawKeybind(&downKey, 1,
          AMMONITE_FORCE_RELEASE, false, keyboardCameraCallback, &directionData[3]);
      }
      if (rightKey != 0) {
        keybindIds[4] = ammonite::input::internal::registerRawKeybind(&rightKey, 1,
          AMMONITE_FORCE_RELEASE, false, keyboardCameraCallback, &directionData[4]);
      }
      if (leftKey != 0) {
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
      if (keybindIds[0] != 0) {
        ammonite::input::internal::unregisterKeybind(keybindIds[0]);
        keybindIds[0] = 0;
      }
      if (keybindIds[1] != 0) {
        ammonite::input::internal::unregisterKeybind(keybindIds[1]);
        keybindIds[1] = 0;
      }
      if (keybindIds[2] != 0) {
        ammonite::input::internal::unregisterKeybind(keybindIds[2]);
        keybindIds[2] = 0;
      }
      if (keybindIds[3] != 0) {
        ammonite::input::internal::unregisterKeybind(keybindIds[3]);
        keybindIds[3] = 0;
      }
      if (keybindIds[4] != 0) {
        ammonite::input::internal::unregisterKeybind(keybindIds[4]);
        keybindIds[4] = 0;
      }
      if (keybindIds[5] != 0) {
        ammonite::input::internal::unregisterKeybind(keybindIds[5]);
        keybindIds[5] = 0;
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
