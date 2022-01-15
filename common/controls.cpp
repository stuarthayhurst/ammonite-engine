#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

//Get fov, height, width and the window
extern float fov;
extern float height, width;
extern float aspectRatio;
extern GLFWwindow* window;

namespace controls {
  namespace {
    //Matrices returned
    glm::mat4 ViewMatrix;
    glm::mat4 ProjectionMatrix;

    //Base sensitivities
    const float baseMovementSpeed = 3.0f;
    const float baseMouseSpeed = 0.005f;

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

    //Points upwards, regardless of direction
    const glm::vec3 absoluteUp = glm::vec3(0, 1, 0);

    //Increase / decrease FoV on scroll (xoffset is unused)
    void scroll_callback(GLFWwindow*, double, double yoffset) {
      //Only zoom if FoV will be between 1 and 90
      float newFov = fov - (yoffset * zoomMultiplier);
      if (newFov > 0 and newFov < 91) {
        fov = newFov;
      }
    }

    //Reset FoV on middle click, (modifier bits are unused)
    void zoom_reset_callback(GLFWwindow*, int button, int action, int) {
      if (button == GLFW_MOUSE_BUTTON_MIDDLE and action == GLFW_PRESS) {
        fov = 45;
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

  void setupControls() {
    //Set mouse callbacks
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, zoom_reset_callback);
  }

  //Handle keyboard and mouse movements, calculate matrices
  void processInput() {
    //glfwGetTime is called only once, the first time this function is called
    static double lastTime = glfwGetTime();

    //Time difference between first and last frame
    double currentTime = glfwGetTime();
    float deltaTime = float(currentTime - lastTime);

    double xpos, ypos;

    //Save last input toggle key state and current input state
    static bool inputBound = true;
    static int lastInputToggleState = GLFW_RELEASE;
    int inputToggleState;

    //Handle toggling input
    inputToggleState = glfwGetKey(window, GLFW_KEY_C);
    if (lastInputToggleState != inputToggleState) {
      if (lastInputToggleState == GLFW_RELEASE) {
        inputBound = !inputBound;
      }

      //Hide and unhide cursor as necessary
      if (inputBound) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      }

      lastInputToggleState = inputToggleState;
    }

    if (inputBound) {
      //Get cursor position and reset for next frame
      glfwGetCursorPos(window, &xpos, &ypos);

      //Compute new orientation
      horizontalAngle = mouseSpeed * float(width / 2 - xpos);
      verticalAngle = mouseSpeed * float(height / 2 - ypos);
    }

    //Direction, Spherical coordinates to Cartesian coordinates conversion
    glm::vec3 direction(
      cos(verticalAngle) * sin(horizontalAngle),
      sin(verticalAngle),
      cos(verticalAngle) * cos(horizontalAngle)
    );

    //Right vector
    glm::vec3 right = glm::vec3(
      sin(horizontalAngle - 3.14f / 2.0f),
      0,
      cos(horizontalAngle - 3.14f / 2.0f)
    );

    //Up vector, perpendicular to both direction and right
    glm::vec3 up = glm::cross(right, direction);

    //Movement
    if (inputBound) {
      if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) { //Move forward
        position += direction * deltaTime * movementSpeed;
      }
      if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) { //Move back
        position -= direction * deltaTime * movementSpeed;
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

    //Projection matrix : 45&deg; Field of View, ratio, display range : 0.1 unit <-> 100 units
    ProjectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 100.0f);
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
