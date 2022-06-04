#include <iostream>
#include <filesystem>
#include <cmath>
#include <vector>
#include <string>

#include <stb/stb_image.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "internal/sharedSettings.hpp"

namespace ammonite {
  namespace windowManager {
    namespace {
      //Window and settings
      GLFWwindow* window;
      bool vsyncEnabled = true;

      //Callback to update height, width and viewport size on window resize
      static void window_size_callback(GLFWwindow*, int width, int height) {
        ammonite::settings::setWidth(width);
        ammonite::settings::setHeight(height);
        glViewport(0, 0, width, height);
      }
    }

    namespace settings {
      void useVsync(bool enabled) {
        glfwSwapInterval(int(enabled));
        vsyncEnabled = enabled;
      }

      bool isVsyncEnabled() {
        return vsyncEnabled;
      }
    }

    namespace setup {
      //Initialise GLFW and setup antialiasing
      bool setupGlfw(int antialiasing) {
        //Setup GLFW
        if (!glfwInit()) {
          std::cerr << "Failed to initialize GLFW" << std::endl;
          return false;
        }

        //Set minimum version to OpenGL 3.2+
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

        //Disable compatibility profile
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        //Set antialiasing level if requested
        if (antialiasing != 0) {
          glfwWindowHint(GLFW_SAMPLES, antialiasing);
          glEnable(GL_MULTISAMPLE);
        }

        return true;
      }

      //Initialise GLEW
      bool setupGlew(GLFWwindow* window) {
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
          std::cerr << "Failed to initialize GLEW" << std::endl;
          return false;
        }

        //Update values when resized
        glfwSetWindowSizeCallback(window, window_size_callback);

        return true;
      }

      //Set input and cursor modes for window
      void setupGlfwInput(GLFWwindow* window) {
        glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
        glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GL_TRUE);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        //Enable raw mouse mption if supported
        if (glfwRawMouseMotionSupported()) {
          glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        }

        //Start polling inputs
        glfwPollEvents();
      }

      void destroyGlfw() {
        glfwTerminate();
      }
    }

    //Create a window and a pointer
    GLFWwindow* createWindow(int width, int height) {
      //Set initial size
      ammonite::settings::setWidth(width);
      ammonite::settings::setHeight(height);

      window = glfwCreateWindow(width, height, "Ammonite Window", NULL, NULL);
      if (window == NULL) {
        std::cerr << "Failed to open window" << std::endl;
        glfwTerminate();
        return nullptr;
      }

      glfwMakeContextCurrent(window);

      //Return the window pointer
      return window;
    }

    void setTitle(GLFWwindow* window, const char* title) {
      if (title != NULL) {
        glfwSetWindowTitle(window, title);
      } else {
        glfwSetWindowTitle(window, "Ammonite Window");
      }
    }

    void useIconDir(GLFWwindow* window, const char* iconDirPath) {
      const std::filesystem::path searchDir{iconDirPath};
      const auto it = std::filesystem::directory_iterator{searchDir};

      //Add all png files to a vector
      std::vector<std::string> pngFiles(0);
      for (auto const& fileName : it) {
        std::filesystem::path filePath{fileName};
        if (filePath.extension() == ".png") {
          pngFiles.push_back(std::string(filePath));
        }
      }

      //Read image data
      GLFWimage images[pngFiles.size()];
      for (unsigned int i = 0; i < pngFiles.size(); i++) {
        images[i].pixels = stbi_load(pngFiles[i].c_str(), &images[i].width, &images[i].height, nullptr, 4);
      }

      //Pass icons to glfw
      if (pngFiles.size() != 0) {
        glfwSetWindowIcon(window, pngFiles.size(), images);
      }

      //Free the data
      for (unsigned int i = 0; i < pngFiles.size(); i++) {
        stbi_image_free(images[i].pixels);
      }

    }

    //Wrapper to create and setup window
    GLFWwindow* setupWindow(int width, int height, int antialiasing, const char* title) {
      //Setup GLFW and antialiasing
      if (!windowManager::setup::setupGlfw(antialiasing)) {
        return nullptr;
      }

      auto window = windowManager::createWindow(width, height);
      if (window == NULL) {
        return nullptr;
      }

      //Set window title
      windowManager::setTitle(window, title);

      //Setup GLEW
      if (!windowManager::setup::setupGlew(window)) {
        return nullptr;
      }

      //Setup input for window
      windowManager::setup::setupGlfwInput(window);

      //Return same values as createWindow()
      return window;
    }
  }
}
