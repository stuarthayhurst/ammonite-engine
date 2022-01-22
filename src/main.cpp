#include <iostream>
#include <cstdlib>
#include <tuple>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ammonite/loadShader.hpp"
#include "ammonite/controls.hpp"
#include "ammonite/windowManager.hpp"

//Program helpers
#include "common/argHandler.hpp"

//OpenGL settings
const int antialiasingLevel = 4;
const float openglVersion = 3.3f;

//Initial width and height
const unsigned short int width = 1024;
const unsigned short int height = 768;

int main(int argc, char* argv[]) {
  //Handle arguments
  int showHelp = arguments::searchArgument(argc, argv, "--help", true, nullptr);
  if (showHelp == 1) {
    std::cout << "Program help: \n"
    " --help:  Display this help page\n"
    " --vsync: Enable / disable VSync (true / false)" << std::endl;
    return EXIT_SUCCESS;
  } else if (showHelp == -1) {
    return EXIT_FAILURE;
  }

  std::string useVsync;
  if (arguments::searchArgument(argc, argv, "--vsync", false, &useVsync) == -1) {
    std::cout << "--vsync requires a value" << std::endl;
    return EXIT_FAILURE;
  }

  auto [window, widthPtr, heightPtr, aspectRatioPtr] = windowManager::setupWindow(width, height, antialiasingLevel, openglVersion, "OpenGL Experiments");
  if (window == NULL) {
    return EXIT_FAILURE;
  }

  //Initialise controls
  controls::setupControls(window, widthPtr, heightPtr, aspectRatioPtr);

  //Set vsync
  if (useVsync == "true") {
    windowManager::settings::useVsync(true);
  } else if (useVsync == "false") {
    windowManager::settings::useVsync(false);
  }

  //Enable culling triangles
  glEnable(GL_CULL_FACE);
  //Enable depth test and only show fragments closer than the previous
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  //Create the VAO
  GLuint VertexArrayID;
  glGenVertexArrays(1, &VertexArrayID);
  glBindVertexArray(VertexArrayID);

  //Create and compile shaders
  GLuint programID = renderer::shaders::loadShaders("shaders/SimpleVertexShader.vert", "shaders/SimpleFragmentShader.frag");
  //Get an ID for the model view projection
  GLuint MatrixID = glGetUniformLocation(programID, "MVP");

  //An array of verticies to draw
  static const GLfloat g_vertex_buffer_data[] = {
    -1.0f,-1.0f,-1.0f, // triangle 1 : begin
    -1.0f,-1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f, // triangle 1 : end
    1.0f, 1.0f,-1.0f, // triangle 2 : begin
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f,-1.0f, // triangle 2 : end
    1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f,-1.0f,
    1.0f,-1.0f,-1.0f,
    1.0f, 1.0f,-1.0f,
    1.0f,-1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
    1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,
    1.0f,-1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f,-1.0f,-1.0f,
    1.0f, 1.0f,-1.0f,
    1.0f,-1.0f,-1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f,-1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f,-1.0f,
    -1.0f, 1.0f,-1.0f,
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
    -1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    1.0f,-1.0f, 1.0f
  };

  //Create a vertex buffer
  GLuint vertexbuffer;
  glGenBuffers(1, &vertexbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
  //Give vertices to OpenGL
  glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

  static float colourVal[12][3] = {
    {0.1, 1.0, 1},
    {0.2, 0.9, 1},
    {0.3, 0.8, 1},
    {0.4, 0.7, 1},
    {0.5, 0.6, 1},
    {0.6, 0.5, 1},
    {0.7, 0.4, 1},
    {0.8, 0.3, 1},
    {0.9, 0.2, 1},
    {1.0, 0.1, 1},
    {0.1, 1.0, 1},
    {0.2, 0.9, 1}
  };

  //Fill faces of cube with colours
  static GLfloat g_colour_buffer_data[12*3*3];
  for (int triangle = 0; triangle < 12; triangle++) {
    for (int v = 0; v < 3; v++) {
      for (int colour = 0; colour < 3; colour++) {
        g_colour_buffer_data[triangle * 9 + (v * 3 + colour)] = colourVal[triangle][colour];
      }
    }
  }

  //Create a colour buffer
  GLuint colourbuffer;
  glGenBuffers(1, &colourbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, colourbuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(g_colour_buffer_data), g_colour_buffer_data, GL_STATIC_DRAW);

  //Framerate variables
  double deltaTime, currentTime;
  double lastTime = glfwGetTime();
  int frameCount = 0;

  //Loop until window closed
  while(glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(window) == 0) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //Every second, output the framerate
    currentTime = glfwGetTime();
    deltaTime = currentTime - lastTime;
    frameCount++;

    if (deltaTime >= 1.0) {
      printf("%f fps", frameCount / deltaTime);
      printf(" (%fms)\n", (deltaTime * 1000) / frameCount);
      lastTime = currentTime;
      frameCount = 0;
    }

    //Use the shaders
    glUseProgram(programID);

    //Process new input since last frame
    controls::processInput();

    //Get current model, view and projection matrices, and compute the MVP matrix
    glm::mat4 ProjectionMatrix = controls::matrix::getProjectionMatrix();
    glm::mat4 ViewMatrix = controls::matrix::getViewMatrix();
    glm::mat4 ModelMatrix = glm::mat4(1.0);
    glm::mat4 mvp = ProjectionMatrix * ViewMatrix * ModelMatrix;

    //Send the transformation to the current shader in "MVP" uniform
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]);

    //Vertex attribute buffer
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
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
    glBindBuffer(GL_ARRAY_BUFFER, colourbuffer);
    glVertexAttribPointer(
      1,
      3,
      GL_FLOAT,
      GL_FALSE,
      0,
      (void*)0
    );

    //Draw the triangles
    glDrawArrays(GL_TRIANGLES, 0, 12*3); //12*3 indicies starting at 0 (12 triangles, 6 squares)
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    //Swap buffers
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  //Cleanup VBO, shader and window
  glDeleteBuffers(1, &vertexbuffer);
  glDeleteBuffers(1, &colourbuffer);
  glDeleteProgram(programID);
  glDeleteVertexArrays(1, &VertexArrayID);
  glfwTerminate();

  return EXIT_SUCCESS;
}
