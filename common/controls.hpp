#include <GLFW/glfw3.h>
extern GLFWwindow* window;

// Include GLM
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
//Horizontal angle : toward -Z
float horizontalAngle = 3.14f;
//Vertical angle : 0, look at the horizon
float verticalAngle = 0.0f;

//Get fov, height and width
extern float fov;
extern float height, width;

float speed = 3.0f; // 3 units / second
float baseMouseSpeed = 0.05f;
float mouseSpeedMultiplier = 1.5f;

float mouseSpeed = baseMouseSpeed * mouseSpeedMultiplier;

void computeMatricesFromInputs() {
  //glfwGetTime is called only once, the first time this function is called
  static double lastTime = glfwGetTime();

  //Time difference between first and last frame
  double currentTime = glfwGetTime();
  float deltaTime = float(currentTime - lastTime);
  cout << deltaTime << endl;

  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);

  //Reset mouse position for next frame
  glfwSetCursorPos(window, width/2, height/2);

  //Compute new orientation
  horizontalAngle += mouseSpeed * deltaTime * float(1024/2 - xpos);
  verticalAngle += mouseSpeed * deltaTime * float(768/2 - ypos);

  //Direction : Spherical coordinates to Cartesian coordinates conversion
  glm::vec3 direction(
    cos(verticalAngle) * sin(horizontalAngle),
    sin(verticalAngle),
    cos(verticalAngle) * cos(horizontalAngle)
  );

  //Right vector
  glm::vec3 right = glm::vec3(
    sin(horizontalAngle - 3.14f/2.0f),
    0,
    cos(horizontalAngle - 3.14f/2.0f)
  );

  //Up vector : perpendicular to both direction and right
  glm::vec3 up = glm::cross(right, direction);

  //Movement
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

  // Projection matrix : 45&deg; Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
  ProjectionMatrix = glm::perspective(glm::radians(fov), width / height, 0.1f, 100.0f);
  // Camera matrix
  ViewMatrix = glm::lookAt(
    position,           // Camera is here
    position+direction, // and looks here : at the same position, plus "direction"
    up                  // Head is up (set to 0,-1,0 to look upside-down)
  );

  //Update lastTime
  lastTime = currentTime;
}
