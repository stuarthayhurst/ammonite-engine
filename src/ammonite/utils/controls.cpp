#include <cmath>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../camera.hpp"
#include "timer.hpp"

#ifdef DEBUG
  #include <iostream>
#endif

namespace ammonite {
  namespace utils {
    namespace controls {
      namespace {
        //Window pointer
        GLFWwindow* window;

        //Base sensitivities and zoom
        const float baseMovementSpeed = 3.0f;
        const float baseMouseSpeed = 0.005f;

        //Sensitivity multipliers (using a negative value will invert movement)
        float movementSpeedMultiplier = 1.0f;
        float mouseSpeedMultiplier = 1.0f;
        float zoomMultiplier = 1.0f;

        //Final sensitivites (zoom only uses the multiplier)
        float movementSpeed = baseMovementSpeed * movementSpeedMultiplier;
        float mouseSpeed = baseMouseSpeed * mouseSpeedMultiplier;

        //Used to find x and y mouse offsets
        double xposLast, yposLast;

        //Current input bind, camera and control states
        bool isInputFocused = true, isCameraActive = true, isControlActive = true;

        //Increase / decrease FoV on scroll (xoffset is unused)
        static void scroll_callback(GLFWwindow*, double, double yoffset) {
          if (isCameraActive) {
            //Only zoom if FoV will be between 1 and 90
            int activeCameraId = ammonite::camera::getActiveCamera();
            float fov = ammonite::camera::getFieldOfView(activeCameraId);
            float newFov = fov - (yoffset * zoomMultiplier);

            if (newFov > 0 and newFov <= 90) {
              ammonite::camera::setFieldOfView(activeCameraId, newFov);
            }
          }
        }

        //Reset FoV on middle click, (modifier bits are unused)
        static void zoom_reset_callback(GLFWwindow*, int button, int action, int) {
          if (isCameraActive) {
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

            //Update viewing angles ('-' corrects camera inversion)
            ammonite::camera::setHorizontal(activeCameraId, horizontalAngle - (mouseSpeed * xoffset));

            //Only accept vertical angle if it won't create an impossible movement
            float newAngle = verticalAngle - (mouseSpeed * yoffset);
            static const float limit = glm::radians(90.0f);
            if (newAngle > limit) { //Vertical max
              verticalAngle = limit;
            } else if (newAngle < -limit) { //Vertical min
              verticalAngle = -limit;
            } else {
              verticalAngle += -mouseSpeed * yoffset;
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

      //Using negative values for movement will invert the axis
      namespace settings {
        void setMovementSpeed(float newMovementSpeed) {
          movementSpeedMultiplier = newMovementSpeed;
          movementSpeed = baseMovementSpeed * movementSpeedMultiplier;
        }

        void setMouseSpeed(float newMouseSpeed) {
          mouseSpeedMultiplier = newMouseSpeed;
          mouseSpeed = baseMouseSpeed * mouseSpeedMultiplier;
        }

        void setZoomSpeed(float newZoomMultiplier) {
          zoomMultiplier = newZoomMultiplier;
        }

        float getMovementSpeed() {
          return movementSpeedMultiplier;
        }

        float getMouseSpeed() {
          return mouseSpeedMultiplier;
        }

        float getZoomSpeed() {
          return zoomMultiplier;
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

      void setupControls(GLFWwindow* newWindow) {
        //Connect window and aspect ratio pointers
        window = newWindow;

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
        return (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS and !glfwWindowShouldClose(window));
      }

      //Handle keyboard and mouse movements, calculate matrices
      void processInput() {
        //Time difference between 2 inputs
        static ammonite::utils::Timer controlTimer;
        float deltaTime = controlTimer.getTime();

        //Poll GLFW for input
        glfwPollEvents();

        if (isControlActive) {
          //Get active camera
          int activeCameraId = ammonite::camera::getActiveCamera();

          //Vector for current direction, without vertical component
          float horizontalAngle = ammonite::camera::getHorizontal(activeCameraId);
          glm::vec3 horizontalDirection(
            std::sin(horizontalAngle),
            0,
            std::cos(horizontalAngle)
          );

          //Right vector, relative to the camera
          glm::vec3 right = glm::vec3(
            std::sin(horizontalAngle - glm::half_pi<float>()),
            0,
            std::cos(horizontalAngle - glm::half_pi<float>())
          );

          //Get the current camera position
          glm::vec3 position = ammonite::camera::getPosition(activeCameraId);

          //Apply movement from inputs
          if (isInputFocused) {
            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) { //Move forward
              position += horizontalDirection * deltaTime * movementSpeed;
            }
            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) { //Move back
              position -= horizontalDirection * deltaTime * movementSpeed;
            }
            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) { //Move right
              position += right * deltaTime * movementSpeed;
            }
            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) { //Move left
              position -= right * deltaTime * movementSpeed;
            }

            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) { //Move up
              position += glm::vec3(0, 1, 0) * deltaTime * movementSpeed;
            }
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) { //Move down
              position -= glm::vec3(0, 1, 0) * deltaTime * movementSpeed;
            }
          }

          //Update the camera position
          ammonite::camera::setPosition(activeCameraId, position);
        }

        //Reset time between inputs
        controlTimer.reset();
      }
    }
  }
}
