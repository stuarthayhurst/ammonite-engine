#include <iostream>
#include <vector>

extern "C" {
  #include <GLFW/glfw3.h>
}

#include "input.hpp"

#include "keycodes.hpp"

#include "../utils/debug.hpp"

/*
 - This file implements GLFW-specific key handling, used by the input layer
*/

namespace ammonite {
  namespace input {
    namespace internal {
      namespace {
        GLFWwindow* windowPtr = nullptr;
        std::vector<KeycodeStatePair> updatedKeys;
      }

      //Window implementation specific internal functions
      namespace {
        //Update the states of tracked keys on input
        void keyCallbackHandler(GLFWwindow*, int rawKeycode, int, int rawAction, int) {
          //Convert GLFW values to enums
          const AmmoniteKeycode keycode = (AmmoniteKeycode)rawKeycode;
          KeyStateEnum action = AMMONITE_RELEASED;
          if (rawAction == GLFW_PRESS || rawAction == GLFW_REPEAT) {
            action = AMMONITE_PRESSED;
          }

          //Track all updated keys
          updatedKeys.push_back({.keycode = keycode, .state = action});

          //Filter out unmapped keys
          KeyStateEnum* const keycodeStatePtr = getKeycodeStatePtr(keycode);
          if (keycodeStatePtr == nullptr) {
            ammoniteInternalDebug << "Keycode '" << keycode << "' not registered" << std::endl;
            return;
          }

          //Debug logging for state changes
          if (*keycodeStatePtr == action) {
            if (action == AMMONITE_PRESSED) {
              ammoniteInternalDebug << "Keycode '" << keycode << "' already held" << std::endl;
            } else {
              ammoniteInternalDebug << "Keycode '" << keycode << "' wasn't held" << std::endl;
            }
          } else {
            if (action == AMMONITE_PRESSED) {
              ammoniteInternalDebug << "Keycode '" << keycode << "' pressed" << std::endl;
            } else {
              ammoniteInternalDebug << "Keycode '" << keycode << "' released" << std::endl;
            }
          }

          //Track the new state
          *keycodeStatePtr = action;
        }
      }

      void setupInputCallback(GLFWwindow* newWindowPtr) {
        windowPtr = newWindowPtr;
        glfwSetKeyCallback(windowPtr, keyCallbackHandler);
      }

      KeyStateEnum getKeyState(AmmoniteKeycode keycode) {
        //Treat key as unpressed if the window isn't ready yet
        if (windowPtr == nullptr) {
          return AMMONITE_RELEASED;
        }

        if (glfwGetKey(windowPtr, keycode) == GLFW_RELEASE) {
          return AMMONITE_RELEASED;
        }

        return AMMONITE_PRESSED;
      }

      std::vector<KeycodeStatePair>* getUpdatedKeys() {
        return &updatedKeys;
      }

      void clearUpdatedKeys() {
        updatedKeys.clear();
      }

      void updateEvents() {
        glfwPollEvents();
      }
    }
  }
}
