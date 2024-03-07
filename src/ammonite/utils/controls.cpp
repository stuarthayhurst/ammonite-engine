#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "../core/inputManager.hpp"
#include "../core/windowManager.hpp"

#include "../internal/internalSettings.hpp"
#include "../camera.hpp"
#include "../enums.hpp"
#include "timer.hpp"

namespace ammonite {
  namespace utils {
    namespace controls {
      namespace {
        //Window pointer
        GLFWwindow* windowPtr;

        //Used to find x and y mouse offsets
        double xposLast, yposLast;
        bool ignoreNextCursor = false;

        //Current camera control states
        bool isCameraActive = true;

        bool* isInputBlockedPtr = ammonite::input::internal::getInputBlockPtr();

        typedef void (*CursorPositionCallback)(GLFWwindow*, double, double);
        CursorPositionCallback activeCursorPositionCallback = nullptr;

        //Increase / decrease FoV on scroll (xoffset is unused)
        static void scrollCallback(GLFWwindow*, double, double yoffset) {
          if (!(*isInputBlockedPtr) and isCameraActive) {
            int activeCameraId = ammonite::camera::getActiveCamera();
            float fov = ammonite::camera::getFieldOfView(activeCameraId);

            static float* zoomSpeedPtr = ammonite::settings::controls::internal::getZoomSpeedPtr();
            static float* fovLimitPtr = ammonite::settings::controls::internal::getFovLimitPtr();

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
              ammonite::camera::setFieldOfView(ammonite::camera::getActiveCamera(), 45.0f);
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

            static float* mouseSpeedPtr =
              ammonite::settings::controls::internal::getMouseSpeedPtr();

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

      void setupControls() {
        //Connect window pointer
        windowPtr = ammonite::window::internal::getWindowPtr();

        //Prepare cursor
        ignoreNextCursor = true;
        glfwSetInputMode(windowPtr, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwGetCursorPos(windowPtr, &xposLast, &yposLast);

        //Set mouse control callbacks
        glfwSetScrollCallback(windowPtr, scrollCallback);
        glfwSetMouseButtonCallback(windowPtr, zoomResetCallback);
        glfwSetCursorPosCallback(windowPtr, cursorPositionCallback);
        activeCursorPositionCallback = cursorPositionCallback;
      }

      void releaseControls() {
        glfwSetScrollCallback(windowPtr, nullptr);
        glfwSetMouseButtonCallback(windowPtr, nullptr);
        glfwSetCursorPosCallback(windowPtr, nullptr);
        activeCursorPositionCallback = nullptr;

        glfwSetInputMode(windowPtr, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

        windowPtr = nullptr;
      }
    }
  }
}
