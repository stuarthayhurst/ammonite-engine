#include <cmath>
#include <map>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../internal/internalSettings.hpp"
#include "../internal/keybindTracker.hpp"
#include "../constants.hpp"
#include "../camera.hpp"
#include "../windowManager.hpp"
#include "timer.hpp"

namespace ammonite {
  namespace utils {
    namespace controls {
      namespace {
        //Window pointer
        GLFWwindow* window;

        //Used to find x and y mouse offsets
        double xposLast, yposLast;

        //Current input bind, camera and control states
        bool isInputFocused = true, isCameraActive = true, isControlActive = true;

        //Access map for keybind stores
        std::map<AmmoniteEnum, int>* keybindTrackerPtr = ammonite::utils::controls::internal::getKeybindTrackerPtr();

        //Increase / decrease FoV on scroll (xoffset is unused)
        static void scroll_callback(GLFWwindow*, double, double yoffset) {
          if (isInputFocused and isCameraActive) {
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
        static void zoom_reset_callback(GLFWwindow*, int button, int action, int) {
          if (isInputFocused and isCameraActive) {
            if (button == GLFW_MOUSE_BUTTON_MIDDLE and action == GLFW_PRESS) {
              ammonite::camera::setFieldOfView(ammonite::camera::getActiveCamera(), 45.0f);
            }
          }
        }

        static void cursor_position_callback(GLFWwindow*, double xpos, double ypos) {
          if (isCameraActive) {
            //Work out distance moved since last movement
            float xoffset = xpos - xposLast;
            float yoffset = ypos - yposLast;

            //Update saved cursor positions
            xposLast = xpos;
            yposLast = ypos;

            //Get current viewing angles
            int activeCameraId = ammonite::camera::getActiveCamera();
            float horizontalAngle = ammonite::camera::getHorizontal(activeCameraId);
            float verticalAngle = ammonite::camera::getVertical(activeCameraId);

            static float* mouseSpeedPtr = ammonite::settings::controls::internal::getMouseSpeedPtr();

            //Update viewing angles ('-' corrects camera inversion)
            ammonite::camera::setHorizontal(activeCameraId, horizontalAngle - (*mouseSpeedPtr * xoffset));

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

        //Helper function to set input state
        static void setInputFocusInternal(bool newisInputFocused) {
          isInputFocused = newisInputFocused;

          //Hide and unhide cursor as necessary
          if (isInputFocused) {
            //Hide cursor and start taking mouse input
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwSetCursorPosCallback(window, cursor_position_callback);
            //Reset saved cursor position to avoid a large jump
            glfwGetCursorPos(window, &xposLast, &yposLast);
          } else {
            //Remove callback and restore cursor
            glfwSetCursorPosCallback(window, NULL);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
          }
        }

        static void window_focus_callback(GLFWwindow*, int focused) {
          //Bind / unbind input with window focus (fixes missing mouse)
          if (!focused) {
            setInputFocusInternal(focused);
          }
        }
      }

      void setCameraActive(bool active) {
        isCameraActive = active;
      }

      void setControlsActive(bool active) {
        isControlActive = active;
      }

      void setInputFocus(bool active) {
        setInputFocusInternal(active);
      }

      bool getCameraActive() {
        return isCameraActive;
      }

      bool getControlsActive() {
        return isControlActive;
      }

      bool getInputFocus() {
        return isInputFocused;
      }

      void setupControls() {
        //Connect window pointer
        window = ammonite::window::getWindowPtr();

        //Set mouse callbacks
        glfwSetScrollCallback(window, scroll_callback);
        glfwSetMouseButtonCallback(window, zoom_reset_callback);
        glfwSetCursorPosCallback(window, cursor_position_callback);

        //Set callback to update input state on window focus
        glfwSetWindowFocusCallback(window, window_focus_callback);

        //Setup initial cursor position
        glfwGetCursorPos(window, &xposLast, &yposLast);
      }

      //Return true if the window should be closed
      bool shouldWindowClose() {
        return (glfwGetKey(window, (*keybindTrackerPtr)[AMMONITE_EXIT]) != GLFW_PRESS and !glfwWindowShouldClose(window));
      }

      //Handle keyboard and mouse movements, calculate matrices
      void processInput() {
        //Time difference between 2 inputs
        static ammonite::utils::Timer controlTimer;
        float deltaTime = controlTimer.getTime();

        //Poll GLFW for input
        glfwPollEvents();

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

        static float* movementSpeedPtr = ammonite::settings::controls::internal::getMovementSpeedPtr();

        //Apply movement from inputs
        if (isInputFocused and isControlActive) {
          if (glfwGetKey(window, (*keybindTrackerPtr)[AMMONITE_FORWARD]) == GLFW_PRESS) { //Move forward
            position += horizontalDirection * deltaTime * *movementSpeedPtr;
          }
          if (glfwGetKey(window, (*keybindTrackerPtr)[AMMONITE_BACK]) == GLFW_PRESS) { //Move back
            position -= horizontalDirection * deltaTime * *movementSpeedPtr;
          }
          if (glfwGetKey(window, (*keybindTrackerPtr)[AMMONITE_RIGHT]) == GLFW_PRESS) { //Move right
            position += right * deltaTime * *movementSpeedPtr;
          }
          if (glfwGetKey(window, (*keybindTrackerPtr)[AMMONITE_LEFT]) == GLFW_PRESS) { //Move left
            position -= right * deltaTime * *movementSpeedPtr;
          }

          if (glfwGetKey(window, (*keybindTrackerPtr)[AMMONITE_UP]) == GLFW_PRESS) { //Move up
            position += glm::vec3(0, 1, 0) * deltaTime * *movementSpeedPtr;
          }
          if (glfwGetKey(window, (*keybindTrackerPtr)[AMMONITE_DOWN]) == GLFW_PRESS) { //Move down
            position -= glm::vec3(0, 1, 0) * deltaTime * *movementSpeedPtr;
          }
        }

        //Update the camera position
        ammonite::camera::setPosition(activeCameraId, position);

        //Reset time between inputs
        controlTimer.reset();
      }
    }
  }
}
