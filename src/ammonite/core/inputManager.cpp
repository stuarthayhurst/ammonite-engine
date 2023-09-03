#include <map>
#include <iostream>

#include <GLFW/glfw3.h>

#include "../utils/debug.hpp"

namespace ammonite {
  namespace input {
    namespace internal {
      namespace {
        struct KeybindData {
          bool toggle;
          void(*callback)(int, int, void*);
          void* userPtr;
        };

        //Track bound keys
        std::map<int, KeybindData> keybindMap;

        //Dispatch user defined code to handle keypress
        static void keyCallbackHandler(GLFWwindow*, int keycode, int, int action, int) {
          if (keybindMap.contains(keycode)) {
            KeybindData keybindData = keybindMap[keycode];

            //Exit if in toggle mode and key is held or released
            if (keybindData.toggle && action != GLFW_PRESS) {
              return;
            }

            keybindData.callback(keycode, action, keybindData.userPtr);
          } else {
            ammoniteInternalDebug << "Keycode '" << keycode << "' not registered" << std::endl;
          }
        }
      }

      //Link to window and set callback
      void setupCallback(GLFWwindow* windowPtr) {
        glfwSetKeyCallback(windowPtr, keyCallbackHandler);
      }

      int registerRawKeybind(int keycode, bool toggle,
                             void(*callback)(int, int, void*), void* userPtr) {
        //Check key isn't already bound
        if (keybindMap.contains(keycode)) {
          ammoniteInternalDebug << "Keycode '" << keycode << "' already registered" << std::endl;
          return -1;
        }

        //Bundle keybind data and add to tracker
        KeybindData keybindData = {
          toggle, callback, userPtr
        };
        keybindMap[keycode] = keybindData;
        return 0;
      }

      void unregisterKeybind(int keycode) {
        //Exit if key wasn't bound
        if (!keybindMap.contains(keycode)) {
          ammoniteInternalDebug << "Can't unregister keycode '" << keycode << "' not registered" << std::endl;
          return;
        }

        //Forget the keybind
        keybindMap.erase(keycode);
      }

      bool isKeybindRegistered(int keycode) {
        return keybindMap.contains(keycode);
      }

      void moveKeybindData(int oldKeycode, int newKeycode) {
        //Copy keybind data and delete binding
        KeybindData oldData = keybindMap[oldKeycode];
        unregisterKeybind(oldKeycode);

        //Register old data with new keycode
        keybindMap[newKeycode] = oldData;
      }
    }
  }
}
