#include <map>
#include <vector>
#include <iostream>

#include <GLFW/glfw3.h>

#include "../utils/debug.hpp"

namespace ammonite {
  namespace input {
    namespace internal {
      namespace {
        struct KeybindData {
          int keycode;
          bool toggle;
          void(*callback)(int, int, void*);
          void* userPtr;
        };

        //Track bound and held keys
        std::map<int, KeybindData> keybindMap;
        std::map<int, KeybindData*> heldKeybindMap;

        //Vectors to track pressed and released keys, with a pending callback
        std::vector<KeybindData*> pressedKeys;
        std::vector<KeybindData*> releasedKeys;

        //Dispatch user defined code to handle keypress
        static void keyCallbackHandler(GLFWwindow*, int keycode, int, int action, int) {
          if (!keybindMap.contains(keycode)) {
            ammoniteInternalDebug << "Keycode '" << keycode << "' not registered" << std::endl;
            return;
          }

          //Track new state for the keybind
          if (action == GLFW_PRESS) {
            //Track newly pressed keys
            pressedKeys.push_back(&keybindMap[keycode]);
          } else if (action == GLFW_RELEASE) {
            //Track released keys, remove from held keybind map
            if (heldKeybindMap.contains(keycode)) {
              heldKeybindMap.erase(keycode);
              releasedKeys.push_back(&keybindMap[keycode]);
            } else {
              ammoniteInternalDebug << "Keycode '" << keycode << "' wasn't held" << std::endl;
            }
          }
        }
      }

      //Execute callbacks for anything that changed, or is held
      void runCallbacks() {
        //Run callbacks for released keys
        for (auto it = releasedKeys.begin(); it != releasedKeys.end(); it++) {
          KeybindData* keybindData = *it;
          //Run callback if it's not a toggle
          if (!(keybindData->toggle)) {
            keybindData->callback(keybindData->keycode, GLFW_RELEASE, keybindData->userPtr);
          }
        }
        releasedKeys.clear();

        //Run callbacks for held keys
        for (auto it = heldKeybindMap.begin(); it != heldKeybindMap.end(); it++) {
          KeybindData* keybindData = it->second;

          //Run callback if it's not a toggle keybind
          if (!(keybindData->toggle)) {
            keybindData->callback(keybindData->keycode, GLFW_REPEAT, keybindData->userPtr);
          }
        }

        //Run callbacks for pressed keys, add to held keybind map
        for (auto it = pressedKeys.begin(); it != pressedKeys.end(); it++) {
          KeybindData* keybindData = *it;
          keybindData->callback(keybindData->keycode, GLFW_PRESS, keybindData->userPtr);
          heldKeybindMap[keybindData->keycode] = keybindData;
        }
        pressedKeys.clear();
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
          keycode, toggle, callback, userPtr
        };
        keybindMap[keycode] = keybindData;
        return 0;
      }

      int unregisterKeybind(int keycode) {
        //Exit if key wasn't bound
        if (!keybindMap.contains(keycode)) {
          ammoniteInternalDebug << "Can't unregister keycode '" << keycode << "' not registered" << std::endl;
          return -1;
        }

        //If the key was held, forget it
        if (heldKeybindMap.contains(keycode)) {
          heldKeybindMap.erase(keycode);
        }

        //Forget the keybind
        keybindMap.erase(keycode);
        return 0;
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
