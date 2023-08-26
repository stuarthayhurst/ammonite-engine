#include <iostream>
#include <filesystem>
#include <cmath>
#include <vector>
#include <string>

#include <stb/stb_image.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "internal/internalSettings.hpp"
#include "graphics/internal/shaderCacheUpdate.hpp"
#include "utils/logging.hpp"
#include "constants.hpp"

namespace ammonite {
  namespace window {
    namespace {
      //Window pointer
      GLFWwindow* window;
      const char* DEFAULT_TITLE = "Ammonite Window";

      AmmoniteEnum requestedContextType = AMMONITE_DEFAULT_CONTEXT;

      //Callback to update height and width on window resize
      static void window_size_callback(GLFWwindow*, int width, int height) {
        ammonite::settings::runtime::internal::setWidth(width);
        ammonite::settings::runtime::internal::setHeight(height);
      }
    }

    namespace internal {
      GLFWwindow* getWindowPtr() {
        return window;
      }
    }

    namespace setup {
      //Initialise GLFW
      bool setupGlfw() {
        //Setup GLFW
        if (!glfwInit()) {
          ammonite::utils::error << "Failed to initialize GLFW" << std::endl;
          return false;
        }

        //Set minimum version to OpenGL 3.2+
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

        //Disable compatibility profile
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        //Set requested context type
        if (requestedContextType == AMMONITE_NO_ERROR_CONTEXT) {
          glfwWindowHint(GLFW_CONTEXT_NO_ERROR, GLFW_TRUE);
        } else if (requestedContextType == AMMONITE_DEBUG_CONTEXT) {
          glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
        }

        return true;
      }

      //Initialise GLEW
      bool setupGlew(GLFWwindow* window) {
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
          ammonite::utils::error << "Failed to initialize GLEW" << std::endl;
          return false;
        }

        //Update values when resized
        glfwSetWindowSizeCallback(window, window_size_callback);

        //Prompt shader cache setup
        ammonite::shaders::internal::updateGLCacheSupport();

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

    void requestContextType(AmmoniteEnum contextType) {
      if (window != NULL) {
        ammonite::utils::warning << "Window already created, ignoring context type request" << std::endl;
        return;
      }

      switch (contextType) {
      case AMMONITE_DEFAULT_CONTEXT:
      case AMMONITE_NO_ERROR_CONTEXT:
      case AMMONITE_DEBUG_CONTEXT:
        requestedContextType = contextType;
        return;
      default:
        return;
      }
    }

    //Create a window and a pointer
    GLFWwindow* createWindow(int width, int height, const char* title) {
      //Set initial size
      ammonite::settings::runtime::internal::setWidth(width);
      ammonite::settings::runtime::internal::setHeight(height);

      window = glfwCreateWindow(width, height, title, NULL, NULL);
      if (window == NULL) {
        ammonite::utils::error << "Failed to open window" << std::endl;
        glfwTerminate();
        return nullptr;
      }

      glfwMakeContextCurrent(window);

      //Return the window pointer
      return window;
    }

    GLFWwindow* createWindow(int width, int height) {
      return createWindow(width, height, DEFAULT_TITLE);
    }

    GLFWwindow* getWindowPtr() {
      return window;
    }

    void setTitle(const char* title) {
      if (title != NULL) {
        glfwSetWindowTitle(window, title);
      } else {
        glfwSetWindowTitle(window, DEFAULT_TITLE);
      }
    }

    void useIconDir(const char* iconDirPath) {
      //Attempt to add all png files to a vector
      std::vector<std::string> pngFiles(0);
      try {
        const std::filesystem::path searchDir{iconDirPath};
        const auto it = std::filesystem::directory_iterator{searchDir};

        for (auto const& fileName : it) {
          std::filesystem::path filePath{fileName};
          if (filePath.extension() == ".png") {
            pngFiles.push_back(std::string(filePath));
          }
        }
      } catch (const std::filesystem::filesystem_error&) {
        ammonite::utils::warning << "Couldn't open '" << iconDirPath << "'" << std::endl;
        return;
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
    GLFWwindow* setupWindow(int width, int height, const char* title) {
      //Setup GLFW
      if (!window::setup::setupGlfw()) {
        return nullptr;
      }

      auto window = window::createWindow(width, height, title);
      if (window == NULL) {
        return nullptr;
      }

      //Setup GLEW
      if (!window::setup::setupGlew(window)) {
        return nullptr;
      }

      //Setup input for window
      window::setup::setupGlfwInput(window);

      //Return same values as createWindow()
      return window;
    }

    GLFWwindow* setupWindow(int width, int height) {
      return setupWindow(width, height, DEFAULT_TITLE);
    }
  }
}
