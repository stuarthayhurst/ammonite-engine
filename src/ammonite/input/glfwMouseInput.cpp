#include <iostream>

extern "C" {
  #include <GLFW/glfw3.h>
}

#include "input.hpp"

#include "../utils/logging.hpp"

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

        bool mouseInputBlocked = false;
        bool ignoreNextCursor = true;
        double xPosLast = 0.0, yPosLast = 0.0;

        GLFWwindow* windowPtr = nullptr;
      }

      //Wrappers to interface with GLFW
      namespace {
        void cursorPositionCallbackWrapper(GLFWwindow*, double xPos, double yPos) {
          if (!mouseInputBlocked && cursorPositionCallback != nullptr) {
            if (ignoreNextCursor) {
              glfwGetCursorPos(windowPtr, &xPosLast, &yPosLast);
              ignoreNextCursor = false;
              return;
            }

            //Work out distance moved since last movement
            const double xDelta = xPos - xPosLast;
            const double yDelta = yPos - yPosLast;

            //Update saved cursor positions
            xPosLast = xPos;
            yPosLast = yPos;

            cursorPositionCallback(xPos, yPos, xDelta, yDelta, cursorUserPtr);
          }
        }

        void mouseButtonCallbackWrapper(GLFWwindow*, int button, int action, int) {
          if (!mouseInputBlocked && mouseButtonCallback != nullptr) {
            KeyStateEnum buttonState = AMMONITE_RELEASED;
            if (action == GLFW_PRESS) {
              buttonState = AMMONITE_PRESSED;
            }

            mouseButtonCallback(button, buttonState, mouseUserPtr);
          }
        }

        void scrollWheelCallbackWrapper(GLFWwindow*, double xOffset, double yOffset) {
          if (!mouseInputBlocked && scrollWheelCallback != nullptr) {
            scrollWheelCallback(xOffset, yOffset, scrollUserPtr);
          }
        }
      }

      void setMouseInputBlock(bool inputBlocked) {
        mouseInputBlocked = inputBlocked;
        ignoreNextCursor = true;

        if (windowPtr == nullptr) {
          ammonite::utils::warning << "Can't set mouse input focus before the window exists" \
                                   << std::endl;
          return;
        }

        //Show or hide cursor depending on the block
        if (inputBlocked) {
          //Restore cursor
          glfwSetInputMode(windowPtr, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
          //Hide cursor
          glfwSetInputMode(windowPtr, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
      }

      bool getMouseInputBlock() {
        return mouseInputBlocked;
      }

      void setupMouseCallback(GLFWwindow* newWindowPtr) {
        windowPtr = newWindowPtr;

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
