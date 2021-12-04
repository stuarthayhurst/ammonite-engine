#include <stdio.h>
#include <stdlib.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include "common/loadShader.h"

int width = 1024;
int height = 768;

char title[] = "Tutorial 4";

//Field of view
float fov = 45;

int main() {
  //Initialise GLFW
  glewExperimental = true; //Needed for core profile
  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW");
    return -1;
  }

  glfwWindowHint(GLFW_SAMPLES, 4); //4x antialiasing
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); //OpenGL 3.3
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //Disable older OpenGL 

  //Open a window and create its OpenGL context
  GLFWwindow* window;
  window = glfwCreateWindow(width, height, title, NULL, NULL);
  if (window == NULL) {
    fprintf(stderr, "Failed to open window");
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window); // Initialize GLEW
  glewExperimental = true; //Needed in core profile
  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to initialize GLEW");
    return -1;
  }

  //Allow input
  glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

  //Enable depth test
  glEnable(GL_DEPTH_TEST);
  //Accept fragment if it closer to the camera than the former one
  glDepthFunc(GL_LESS);

  //Create the VAO
  GLuint VertexArrayID;
  glGenVertexArrays(1, &VertexArrayID);
  glBindVertexArray(VertexArrayID);

  //Create and compile shaders
  GLuint programID = LoadShaders("shaders/SimpleVertexShader.vert", "shaders/SimpleFragmentShader.frag");


  //Projection matrix : 45° Field of View, aspect ratio, display range : 0.1 unit <-> 100 units
  glm::mat4 Projection = glm::perspective(glm::radians(fov), (float) width / (float)height, 0.1f, 100.0f);

  //Camera matrix
  glm::mat4 View = glm::lookAt(
    glm::vec3(4,3,3), //Camera is at (4,3,3), in World Space
    glm::vec3(0,0,0), //and looks at the origin
    glm::vec3(0,1,0)  //Head is up (set to 0,-1,0 to look upside-down)
  );

  //Model matrix : an identity matrix (model be at the origin)
  glm::mat4 Model = glm::mat4(1.0f);
  //Our ModelViewProjection : multiplication of our 3 matrices
  glm::mat4 mvp = Projection * View * Model; // Remember, matrix multiplication is the other way around

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

  static GLfloat g_color_buffer_data[12*3*3];
  for (int triangle = 0; triangle < 12; triangle++) {
    for (int v = 0; v < 3; v++) {
      for (int colour = 0; colour < 3; colour++) {
        g_color_buffer_data[triangle * 9 + (v * 3 + colour)] = colourVal[triangle][colour];
      }
    }
  }

  //Create a colour buffer
  GLuint colorbuffer;
  glGenBuffers(1, &colorbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);

  //Loop until window closed
  while(glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(window) == 0) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //Use the shaders
    glUseProgram(programID);
    //Send the transformation to the current shader in "MVP" uniform
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]);

    //1st attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glVertexAttribPointer(
      0,        //attribute 0, must match the layout in the shader
      3,        //size
      GL_FLOAT, //type
      GL_FALSE, //normalized?
      0,        //stride
      (void*)0  //array buffer offset
    );

    // 2nd attribute buffer : colors
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glVertexAttribPointer(
      1,        //attribute 1, must match the layout in the shader
      3,        //size
      GL_FLOAT, //type
      GL_FALSE, //normalized?
      0,        //stride
      (void*)0  //array buffer offset
    );

    //Draw triangle
    glDrawArrays(GL_TRIANGLES, 0, 12*3); //12*3 indicies starting at 0 (12 triangles, 6 squares)
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    //Swap buffers
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  //Cleanup VBO, shader and window
  glDeleteBuffers(1, &vertexbuffer);
  glDeleteBuffers(1, &colorbuffer);
  glDeleteProgram(programID);
  glDeleteVertexArrays(1, &VertexArrayID);
  glfwTerminate();

  return 0;
}
