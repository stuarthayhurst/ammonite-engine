#include <cstdio>
#include <cmath>
#include <tuple>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace windowManager {
  namespace {
    //Window and info
    GLFWwindow* window;
    int width, height;
    float aspectRatio;

    //Callback to update height and width and viewport size on window resize
    void window_size_callback(GLFWwindow*, int newWidth, int newHeight) {
      width = newWidth;
      height = newHeight;
      aspectRatio = float(width) / float(height);
      glViewport(0, 0, width, height);
    }
  }

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
    int setupGlew(GLFWwindow* window) {
      glewExperimental = true;
      if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
      }

      //Update values when resized
      glfwSetWindowSizeCallback(window, window_size_callback);

      return 0;
    }

    //Set input and cursor modes for window
    void setupGlfwInput(GLFWwindow* window) {
      glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
      glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GL_TRUE);
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
  }

  //Create a window and return pointers to the width, height and aspect ratio
  std::tuple<GLFWwindow*, int*, int*, float*> createWindow(int newWidth, int newHeight, const char title[]) {
    //Set initial size and aspect ratio values
    width = newWidth;
    height = newHeight;
    aspectRatio = float(width) / float(height);

    window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (window == NULL) {
      fprintf(stderr, "Failed to open window\n");
      glfwTerminate();
      return {NULL, nullptr, nullptr, nullptr};
    }

    glfwMakeContextCurrent(window);

    //Return the pointers
    return {window, &width, &height, &aspectRatio};
  }
}
