#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

glm::mat4 ViewMatrix;
glm::mat4 ProjectionMatrix;

glm::mat4 getViewMatrix() {
  return ViewMatrix;
}
glm::mat4 getProjectionMatrix() {
  return ProjectionMatrix;
}

//Starting position
glm::vec3 position = glm::vec3(0, 0, 5);
//Horizontal angle, toward -Z
float horizontalAngle = 3.14f;
//Vertical angle, 0, look at the horizon
float verticalAngle = 0.0f;

//Get fov, height, width and the window
extern float fov;
extern float height, width;
extern float aspectRatio;
extern GLFWwindow* window;

//Set movement and mouse speed
float speed = 3.0f; //3 units per second
const float baseMouseSpeed = 0.005f;
const float mouseSpeedMultiplier = 1.0f;
const float mouseSpeed = baseMouseSpeed * mouseSpeedMultiplier;

//Points upwards, regardless of direction
const glm::vec3 absoluteUp = glm::vec3(0, 1, 0);

void computeMatricesFromInputs() {
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

  inputToggleState = glfwGetKey(window, GLFW_KEY_C);

  //Handle toggling input
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
  if (inputBound == true) {
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) { //Move forward
      position += direction * deltaTime * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) { //Move back
      position -= direction * deltaTime * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) { //Move right
      position += right * deltaTime * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) { //Move left
      position -= right * deltaTime * speed;
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) { //Move up
      position += absoluteUp * deltaTime * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) { //Move down
      position -= absoluteUp * deltaTime * speed;
    }
  }

  //Projection matrix : 45&deg; Field of View, ratio, display range : 0.1 unit <-> 100 units
  ProjectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 100.0f);
  //Camera matrix
  ViewMatrix = glm::lookAt(
    position,           //Camera is here
    position+direction, //Looks here (at the same position, plus "direction")
    up                  //Up
  );

  //Update lastTime
  lastTime = currentTime;
}
