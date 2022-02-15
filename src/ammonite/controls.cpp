#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifdef DEBUG
  #include <iostream>
#endif

namespace ammonite {
  namespace controls {
    namespace {
      //Matrices returned
      glm::mat4 ViewMatrix;
      glm::mat4 ProjectionMatrix;

      //Pointers for window and info
      GLFWwindow* window;
      int *width, *height;
      float* aspectRatio;

      //Base sensitivities and zoom
      const float baseMovementSpeed = 3.0f;
      const float baseMouseSpeed = 0.005f;
      float fov = 45;

      //Sensitivity multipliers (using a negative value will invert movement)
      float movementSpeedMultiplier = 1.0f;
      float mouseSpeedMultiplier = 1.0f;
      float zoomMultiplier = 1.0f;

      //Final sensitivites (zoom only uses the multiplier)
      float movementSpeed = baseMovementSpeed * movementSpeedMultiplier;
      float mouseSpeed = baseMouseSpeed * mouseSpeedMultiplier;

      //Position, start looking towards the horizon at -Z
      glm::vec3 position = glm::vec3(0, 0, 5);
      float horizontalAngle = 3.14f;
      float verticalAngle = 0.0f;

      //Used to find x and y mouse offsets
      double xposLast, yposLast;

      //Current input bind state
      bool inputBound = true;

      //Vectors to ignore certain axis (absoluteUp -> only y-axis)
      const glm::vec3 absoluteUp = glm::vec3(0, 1, 0);

      //Increase / decrease FoV on scroll (xoffset is unused)
      static void scroll_callback(GLFWwindow*, double, double yoffset) {
        //Only zoom if FoV will be between 1 and 90
        float newFov = fov - (yoffset * zoomMultiplier);
        if (newFov > 0 and newFov <= 90) {
          fov = newFov;
        }
      }

      //Reset FoV on middle click, (modifier bits are unused)
      static void zoom_reset_callback(GLFWwindow*, int button, int action, int) {
        if (button == GLFW_MOUSE_BUTTON_MIDDLE and action == GLFW_PRESS) {
          fov = 45;
        }
      }

      static void cursor_position_callback(GLFWwindow*, double xpos, double ypos) {
        //Work out distance moved since last movement
        float xoffset = xpos - xposLast;
        float yoffset = ypos - yposLast;

        //Update saved cursor positions
        xposLast = xpos;
        yposLast = ypos;

        //Update viewing angles, - corrects camera inversion
        horizontalAngle += -mouseSpeed * xoffset;

        //Only accept vertical angle if it won't create an impossible movement
        float newAngle = verticalAngle + (-mouseSpeed * yoffset);
        static const float limit = glm::radians(90.0f);
        if (newAngle > limit) { //Vertical max
          verticalAngle = limit;
        } else if (newAngle < -limit) { //Vertical min
          verticalAngle = -limit;
        } else {
          verticalAngle += -mouseSpeed * yoffset;
        }
      }
    }

    //Helper functions
    namespace {
      static void setInputBound(bool newInputBound) {
        inputBound = newInputBound;

        //Hide and unhide cursor as necessary
        if (inputBound) {
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
    }

    namespace matrix {
      glm::mat4 getViewMatrix() {
        return ViewMatrix;
      }

      glm::mat4 getProjectionMatrix() {
        return ProjectionMatrix;
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

    void setupControls(GLFWwindow* newWindow, int* widthAddr, int* heightAddr, float* aspectRatioAddr) {
      //Connect window, width, height and aspect ratio pointers
      window = newWindow;
      width = widthAddr;
      height = heightAddr;
      aspectRatio = aspectRatioAddr;

      //Set mouse callbacks
      glfwSetScrollCallback(window, scroll_callback);
      glfwSetMouseButtonCallback(window, zoom_reset_callback);
      glfwSetCursorPosCallback(window, cursor_position_callback);

      //Setup initial cursor position
      glfwGetCursorPos(window, &xposLast, &yposLast);
    }

    //Handle keyboard and mouse movements, calculate matrices
    void processInput() {
      //glfwGetTime is called only once, the first time this function is called
      static double lastTime = glfwGetTime();

      //Time difference between first and last frame
      double currentTime = glfwGetTime();
      float deltaTime = float(currentTime - lastTime);

      //Save last input toggle key state and current input state
      static int lastInputToggleState = GLFW_RELEASE;
      int inputToggleState;

      //Handle toggling input
      inputToggleState = glfwGetKey(window, GLFW_KEY_C);
      if (lastInputToggleState != inputToggleState) {
        if (lastInputToggleState == GLFW_RELEASE) {
          inputBound = !inputBound;
        }

        //Update input state
        setInputBound(inputBound);

        lastInputToggleState = inputToggleState;
      }

      //Vector for current direction, without vertical component
      glm::vec3 horizontalDirection(
        sin(horizontalAngle),
        0,
        cos(horizontalAngle)
      );

      //Right vector, relative to the camera
      glm::vec3 right = glm::vec3(
        sin(horizontalAngle - 3.14f / 2.0f),
        0,
        cos(horizontalAngle - 3.14f / 2.0f)
      );

      //Apply movement from inputs
      if (inputBound) {
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
          position += absoluteUp * deltaTime * movementSpeed;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) { //Move down
          position -= absoluteUp * deltaTime * movementSpeed;
        }
      }

    //Vector for current direction faced
      glm::vec3 direction(
        cos(verticalAngle) * sin(horizontalAngle),
        sin(verticalAngle),
        cos(verticalAngle) * cos(horizontalAngle)
      );

      //Up vector, relative to the camera
      glm::vec3 up = glm::cross(right, direction);

      //Projection matrix: Field of view, aspect ratio, display range
      ProjectionMatrix = glm::perspective(glm::radians(fov), *aspectRatio, 0.1f, 100.0f);
      //Camera matrix
      ViewMatrix = glm::lookAt(
        position,             //Camera is here
        position + direction, //Looks here (at the same position, plus "direction")
        up                    //Up
      );

      //Update lastTime
      lastTime = currentTime;
    }
  }
}
