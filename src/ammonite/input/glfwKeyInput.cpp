#include <iostream>

extern "C" {
  #include <GLFW/glfw3.h>
}

#include "input.hpp"

#include "../utils/debug.hpp"

namespace ammonite {
  namespace input {
    namespace internal {
      namespace {
        GLFWwindow* windowPtr = nullptr;
      }

      //Window implementation specific internal functions
      namespace {
        //Update the states of tracked keys on input
        void keyCallbackHandler(GLFWwindow*, int keycode, int, int action, int) {
          //Filter out unmapped keys
          KeyStateEnum* keycodeStatePtr = getKeycodeStatePtr(keycode);
          if (keycodeStatePtr == nullptr) {
            ammoniteInternalDebug << "Keycode '" << keycode << "' not registered" << std::endl;
            return;
          }

          //Track new state for the keycode
          if (action == GLFW_PRESS) {
            if (*keycodeStatePtr == AMMONITE_PRESSED) {
              ammoniteInternalDebug << "Keycode '" << keycode << "' already held" << std::endl;
            }

            *keycodeStatePtr = AMMONITE_PRESSED;
          } else if (action == GLFW_RELEASE) {
            if (*keycodeStatePtr == AMMONITE_RELEASED) {
              ammoniteInternalDebug << "Keycode '" << keycode << "' wasn't held" << std::endl;
            }

            *keycodeStatePtr = AMMONITE_RELEASED;
          }
        }
      }

      void setupInputCallback(GLFWwindow* newWindowPtr) {
        windowPtr = newWindowPtr;
        glfwSetKeyCallback(windowPtr, keyCallbackHandler);
      }

      KeyStateEnum getKeyState(int keycode) {
        //Treat key as unpressed if the window isn't ready yet
        if (windowPtr == nullptr) {
          return AMMONITE_RELEASED;
        }

        if (glfwGetKey(windowPtr, keycode) == GLFW_RELEASE) {
          return AMMONITE_RELEASED;
        }

        return AMMONITE_PRESSED;
      }
    }
  }
}






