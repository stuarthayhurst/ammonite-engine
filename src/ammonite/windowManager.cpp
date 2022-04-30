#include <iostream>
#include <filesystem>
#include <cmath>
#include <tuple>
#include <vector>
#include <string>

#include <stb/stb_image.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace ammonite {
  namespace windowManager {
    namespace {
      //Constants
      const float MIN_OPENGL = 3.2f;

      //Window and info
      GLFWwindow* window;
      int width, height;
      float aspectRatio;

      bool vsyncEnabled = true;

      //Callback to update height and width and viewport size on window resize
      static void window_size_callback(GLFWwindow*, int newWidth, int newHeight) {
        width = newWidth;
        height = newHeight;
        aspectRatio = float(width) / float(height);
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
      //Initialise glfw, setup antialiasing and OpenGL version
      bool setupGlfw(int antialiasing, float openglVersion) {
        //Setup GLFW
        if (!glfwInit()) {
          std::cerr << "Failed to initialize GLFW" << std::endl;
          return false;
        }

        //If provided, split openglVersion into major and minor components and set version
        if (openglVersion >= MIN_OPENGL) {
          const short int openglMajor = std::floor(openglVersion);
          const short int openglMinor = std::round((openglVersion - openglMajor) * 10);

          //Set the requested version
          glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, openglMajor);
          glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, openglMinor);

          //Disable older OpenGL versions
          glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        } else if (openglVersion != 0) {
          std::cout << "Minimum OpenGL version is " << MIN_OPENGL << ", version " << openglVersion << " is unsupported" << std::endl;
        }

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
    }

    //Create a window and return pointers to the width, height and aspect ratio
    std::tuple<GLFWwindow*, int*, int*, float*> createWindow(int newWidth, int newHeight) {
      //Set initial size and aspect ratio values
      width = newWidth;
      height = newHeight;
      aspectRatio = float(width) / float(height);

      window = glfwCreateWindow(width, height, "Ammonite Window", NULL, NULL);
      if (window == NULL) {
        std::cerr << "Failed to open window" << std::endl;
        glfwTerminate();
        return {NULL, nullptr, nullptr, nullptr};
      }

      glfwMakeContextCurrent(window);

      //Return the pointers
      return {window, &width, &height, &aspectRatio};
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
    std::tuple<GLFWwindow*, int*, int*, float*> setupWindow(int newWidth, int newHeight, int antialiasing, float openglVersion, const char* title) {
      //Setup GLFW and OpenGL version / antialiasing
      if (!windowManager::setup::setupGlfw(antialiasing, openglVersion)) {
        return {NULL, nullptr, nullptr, nullptr};
      }

      auto [window, widthPtr, heightPtr, aspectRatioPtr] = windowManager::createWindow(newWidth, newHeight);
      if (window == NULL) {
        return {NULL, nullptr, nullptr, nullptr};
      }

      //Set window title
      windowManager::setTitle(window, title);

      //Setup GLEW
      if (!windowManager::setup::setupGlew(window)) {
        return {NULL, nullptr, nullptr, nullptr};
      }

      //Setup input for window
      windowManager::setup::setupGlfwInput(window);

      //Return same values as createWindow()
      return {window, &width, &height, &aspectRatio};
    }
  }
}
