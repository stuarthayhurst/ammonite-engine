extern "C" {
  #include <GLFW/glfw3.h>
}

#include "input.hpp"

namespace ammonite {
  namespace input {
    namespace internal {
      namespace {
        AmmoniteCursorCallback cursorPositionCallback = nullptr;
        AmmoniteButtonCallback mouseButtonCallback = nullptr;
        AmmoniteScrollCallback scrollWheelCallback = nullptr;

        void* cursorUserPtr = nullptr;
        void* mouseUserPtr = nullptr;
        void* scrollUserPtr = nullptr;
      }

      //Wrappers to interface with GLFW
      namespace {
        void cursorPositionCallbackWrapper(GLFWwindow*, double xPos, double yPos) {
          if (cursorPositionCallback != nullptr) {
            cursorPositionCallback(xPos, yPos, cursorUserPtr);
          }
        }

        void mouseButtonCallbackWrapper(GLFWwindow*, int button, int action, int) {
          if (mouseButtonCallback != nullptr) {
            KeyStateEnum buttonState = AMMONITE_RELEASED;
            if (action == GLFW_PRESS) {
              buttonState = AMMONITE_PRESSED;
            }

            mouseButtonCallback(button, buttonState, mouseUserPtr);
          }
        }

        void scrollWheelCallbackWrapper(GLFWwindow*, double xOffset, double yOffset) {
          if (scrollWheelCallback != nullptr) {
            scrollWheelCallback(xOffset, yOffset, scrollUserPtr);
          }
        }
      }

      void setupMouseCallback(GLFWwindow* windowPtr) {
        glfwSetCursorPosCallback(windowPtr, cursorPositionCallbackWrapper);
        glfwSetMouseButtonCallback(windowPtr, mouseButtonCallbackWrapper);
        glfwSetScrollCallback(windowPtr, scrollWheelCallbackWrapper);
      }

      void setCursorPositionCallback(AmmoniteCursorCallback callback, void* userPtr) {
        cursorPositionCallback = callback;
        cursorUserPtr = userPtr;
      }

      void setMouseButtonCallback(AmmoniteButtonCallback callback, void* userPtr) {
        mouseButtonCallback = callback;
        mouseUserPtr = userPtr;
      }

      void setScrollWheelCallback(AmmoniteScrollCallback callback, void* userPtr) {
        scrollWheelCallback = callback;
        scrollUserPtr = userPtr;
      }
    }
  }
}
