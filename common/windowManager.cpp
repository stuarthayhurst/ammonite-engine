#include <stdio.h>
#include <cmath>
#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace windowManager {
  namespace setup {
    //Initialise glfw, setup antialiasing and OpenGL version
    int setupGlfw(int antialiasing, float openglVersion) {
      //Setup GLFW
      if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW");
        return -1;
      }

      //Split openglVersion into major and minor components
      const unsigned short int openglMajor = std::floor(openglVersion);
      const unsigned short int openglMinor = std::ceil((openglVersion - openglMajor) * 10);

      //Setup OpenGL version and antialiasing
      glfwWindowHint(GLFW_SAMPLES, antialiasing);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, openglMajor);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, openglMinor);
      //Disable older OpenGL
      glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

      return 0;
    }

    //Initialise GLEW
    int setupGlew() {
      glewExperimental = true;
      if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
      }

      return 0;
    }

    //Set input and cursor modes for window
    void setupGlfwInput(GLFWwindow* window) {
      glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
      glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GL_TRUE);
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
  }

  //Create the glfw window
  GLFWwindow* createWindow(int width, int height, const char title[]) {
    static GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (window == NULL) {
      fprintf(stderr, "Failed to open window\n");
      glfwTerminate();
      return NULL;
    }

    glfwMakeContextCurrent(window);

    return window;
  }
}
