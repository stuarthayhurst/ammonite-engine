#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../internal/internalSettings.hpp"

#include "../utils/debug.hpp"
#include "../constants.hpp"

namespace ammonite {
  namespace window {
    namespace internal {
      namespace {
        GLFWwindow* windowPtr = nullptr;
        AmmoniteEnum requestedContextType = AMMONITE_DEFAULT_CONTEXT;

        //Callback to update height and width on window resize
        static void window_size_callback(GLFWwindow*, int width, int height) {
          ammonite::settings::runtime::internal::setWidth(width);
          ammonite::settings::runtime::internal::setHeight(height);
        }
      }

      GLFWwindow* getWindowPtr() {
        return windowPtr;
      }

      int setupGlfw() {
        if (!glfwInit()) {
          return -1;
        }

        //Set minimum version to OpenGL 3.2+
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

        //Disable compatibility profile
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        //Set requested context type
        if (requestedContextType == AMMONITE_NO_ERROR_CONTEXT) {
          ammoniteInternalDebug << "Creating window with AMMONITE_NO_ERROR_CONTEXT" << std::endl;
          glfwWindowHint(GLFW_CONTEXT_NO_ERROR, GLFW_TRUE);
        } else if (requestedContextType == AMMONITE_DEBUG_CONTEXT) {
          ammoniteInternalDebug << "Creating window with AMMONITE_DEBUG_CONTEXT" << std::endl;
          glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
        }

        return 0;
      }

      int setupGlew() {
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
          return -1;
        }

        //Update values when resized
        glfwSetWindowSizeCallback(windowPtr, window_size_callback);

        return 0;
      }

      //Set input and cursor modes for window
      void setupGlfwInput() {
        glfwSetInputMode(windowPtr, GLFW_STICKY_KEYS, GL_TRUE);
        glfwSetInputMode(windowPtr, GLFW_STICKY_MOUSE_BUTTONS, GL_TRUE);
        glfwSetInputMode(windowPtr, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        //Enable raw mouse motion if supported
        if (glfwRawMouseMotionSupported()) {
          glfwSetInputMode(windowPtr, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        }

        //Start polling inputs
        glfwPollEvents();
      }

      void setContextType(AmmoniteEnum contextType) {
        requestedContextType = contextType;
      }

      GLFWwindow* createWindow(int width, int height, const char* title) {
        ammonite::settings::runtime::internal::setWidth(width);
        ammonite::settings::runtime::internal::setHeight(height);

        windowPtr = glfwCreateWindow(width, height, title, NULL, NULL);
        if (windowPtr == NULL) {
          glfwTerminate();
          return nullptr;
        }

        glfwMakeContextCurrent(windowPtr);
        return windowPtr;
      }

      void destroyGlfw() {
        glfwTerminate();
      }
    }
  }
}
