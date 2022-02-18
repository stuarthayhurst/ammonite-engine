#include <iostream>
#include <cstdlib>
#include <tuple>
#include <string>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ammonite/ammonite.hpp"
#include "common/argHandler.hpp"

//Initial width and height
const unsigned short int width = 1024;
const unsigned short int height = 768;

void printMetrics(int frameCount, double deltaTime) {
  printf("%.2f fps", frameCount / deltaTime);
  printf(" (%fms)\n", (deltaTime * 1000) / frameCount);
}

int main(int argc, char* argv[]) {
  //Handle arguments
  const int showHelp = arguments::searchArgument(argc, argv, "--help", true, nullptr);
  if (showHelp == 1) {
    std::cout << "Program help: \n"
    " --help:       Display this help page\n"
    " --benchmark:  Start a benchmark\n"
    " --vsync:      Enable / disable VSync (true / false)" << std::endl;
    return EXIT_SUCCESS;
  } else if (showHelp == -1) {
    return EXIT_FAILURE;
  }

  const bool useBenchmark = arguments::searchArgument(argc, argv, "--benchmark", true, nullptr);

  std::string useVsync;
  if (arguments::searchArgument(argc, argv, "--vsync", false, &useVsync) == -1) {
    std::cout << "--vsync requires a value" << std::endl;
    return EXIT_FAILURE;
  }

  auto [window, widthPtr, heightPtr, aspectRatioPtr] = ammonite::windowManager::setupWindow(width, height, 4, 0, "OpenGL Experiments");
  if (window == NULL) {
    return EXIT_FAILURE;
  }

  //Initialise controls
  ammonite::controls::setupControls(window, widthPtr, heightPtr, aspectRatioPtr);

  //Set vsync (disable if benchmarking)
  if (useVsync == "false" or useBenchmark == true) {
    ammonite::windowManager::settings::useVsync(false);
  } else if (useVsync == "true") {
    ammonite::windowManager::settings::useVsync(true);
  }

  //Enable culling triangles
  glEnable(GL_CULL_FACE);
  //Enable depth test and only show fragments closer than the previous
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  //Create the VAO
  GLuint vertexArrayId;
  glGenVertexArrays(1, &vertexArrayId);
  glBindVertexArray(vertexArrayId);

  //Shader paths and types to create program
  const std::string shaderPaths[2] = {
    "shaders/TextureVertexShader.vert",
    "shaders/TextureFragmentShader.frag"
  };
  const int shaderTypes[2] = {
    GL_VERTEX_SHADER,
    GL_FRAGMENT_SHADER
  };
  int shaderCount = sizeof(shaderPaths) / sizeof(shaderPaths[0]);

  //Enable binary caching
  ammonite::shaders::useProgramCache("cache");

  //Create program from shaders
  bool success = true;
  ammonite::utils::timer performanceTimer;
  GLuint programId = ammonite::shaders::createProgram(shaderPaths, shaderTypes, shaderCount, &success, "program");
  if (!success) {
    std::cerr << "Program creation failed" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Loaded shaders in: " << performanceTimer.getTime() << "s" << std::endl;

  //Get an ID for the model view projection
  GLuint matrixId = glGetUniformLocation(programId, "MVP");

  //Load model
  performanceTimer.reset();
  std::vector<glm::vec3> vertices, normals;
  std::vector<glm::vec2> uvs;
  success = ammonite::models::loadObject("assets/viking_room.obj", vertices, uvs, normals);

  std::cout << "Loaded models in: " << performanceTimer.getTime() << "s (" << vertices.size() << " vertices)" << std::endl;

  //Create a vertex buffer
  GLuint vertexBuffer;
  glGenBuffers(1, &vertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  //Give vertices to OpenGL
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

  //Load the texture
  GLuint textureId = ammonite::textures::loadTexture("assets/viking_room.png", &success);
  if (!success) {
    ammonite::shaders::eraseShaders();
    glDeleteProgram(programId);
    return EXIT_FAILURE;
  }

  //Create a texture buffer
  GLuint textureBuffer;
  glGenBuffers(1, &textureBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, textureBuffer);
  glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);

  //Use the shaders
  glUseProgram(programId);

  //Performance metrics setup
  ammonite::utils::timer benchmarkTimer;
  performanceTimer.reset();
  double deltaTime;
  long totalFrames = 0;
  int frameCount = 0;

  //Loop until window closed
  while(glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS and !glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //Update time and frame counters every frame
    frameCount++;
    totalFrames++;
    deltaTime = performanceTimer.getTime();

    //Every second, output the framerate
    if (deltaTime >= 1.0) {
      printMetrics(frameCount, deltaTime);
      performanceTimer.reset();
      frameCount = 0;
    }

    //Process new input since last frame
    ammonite::controls::processInput();

    //Get current model, view and projection matrices, and compute the MVP matrix
    glm::mat4 projectionMatrix = ammonite::controls::matrix::getProjectionMatrix();
    glm::mat4 viewMatrix = ammonite::controls::matrix::getViewMatrix();
    static const glm::mat4 modelMatrix = glm::mat4(1.0);
    glm::mat4 mvp = projectionMatrix * viewMatrix * modelMatrix;

    //Send the transformation to the current shader in "MVP" uniform
    glUniformMatrix4fv(matrixId, 1, GL_FALSE, &mvp[0][0]);

    //Vertex attribute buffer
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glVertexAttribPointer(
      0,        //shader location
      3,        //size
      GL_FLOAT, //type
      GL_FALSE, //normalized
      0,        //stride
      (void*)0  //array buffer offset
    );

    //Colour attribute buffer
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, textureBuffer);
    glVertexAttribPointer(
      1,
      2,
      GL_FLOAT,
      GL_FALSE,
      0,
      (void*)0
    );

    //Draw the triangles
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    //Swap buffers
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  //Output benchmark score
  if (useBenchmark) {
    std::cout << "\nBenchmark complete:" << std::endl;
    std::cout << "  Average fps: ";
    printMetrics(totalFrames, benchmarkTimer.getTime());
  }

  //Cleanup VBO, shaders and window
  glDeleteBuffers(1, &vertexBuffer);
  glDeleteBuffers(1, &textureBuffer);
  ammonite::shaders::eraseShaders();
  glDeleteProgram(programId);
  glDeleteTextures(1, &textureId);
  glDeleteVertexArrays(1, &vertexArrayId);
  glfwTerminate();

  return EXIT_SUCCESS;
}
