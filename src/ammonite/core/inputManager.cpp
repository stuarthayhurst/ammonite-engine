#include <map>
#include <vector>
#include <iostream>

#include <GLFW/glfw3.h>

#include "../constants.hpp"
#include "../utils/debug.hpp"

namespace ammonite {
  namespace input {
    namespace internal {
      namespace {
        struct KeybindData {
          int keycode;
          AmmoniteEnum overrideMode;
          bool toggle;
          void(*callback)(int, int, void*);
          void* userPtr;
        };

        bool isInputBlocked = false;

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

          KeybindData* keybindData = &keybindMap[keycode];

          //Handle input block and override modes
          if (isInputBlocked) {
            switch (keybindData->overrideMode) {
            case AMMONITE_ALLOW_OVERRIDE: //Allow keypress
              break;
            case AMMONITE_ALLOW_RELEASE: //Allow keypress if released, and was previously tracked
              if (action == GLFW_RELEASE && heldKeybindMap.contains(keycode)) {
                break;
              } else {
                ammoniteInternalDebug << "Keycode '" << keycode << "' blocked" << std::endl;
                return;
              }
            case AMMONITE_FORCE_RELEASE: //Reject keypress
            case AMMONITE_RESPECT_BLOCK:
              ammoniteInternalDebug << "Keycode '" << keycode << "' blocked" << std::endl;
              return;
            default: //Unhandled override, send debug
              ammoniteInternalDebug << "Keycode '" << keycode
                                    << "' has unexpected override mode" << std::endl;
              return;
            }
          }

          //Track new state for the keybind
          if (action == GLFW_PRESS) {
            //Track newly pressed keys
            if (!heldKeybindMap.contains(keycode)) {
              pressedKeys.push_back(keybindData);
            } else {
              ammoniteInternalDebug << "Keycode '" << keycode << "' already held" << std::endl;
            }
          } else if (action == GLFW_RELEASE) {
            //Track released keys, remove from held keybind map
            if (heldKeybindMap.contains(keycode)) {
              releasedKeys.push_back(keybindData);
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
          heldKeybindMap.erase(keybindData->keycode);
        }
        releasedKeys.clear();

        //Run callbacks for held keys
        std::vector<KeybindData*> forceReleaseKeys;
        for (auto it = heldKeybindMap.begin(); it != heldKeybindMap.end(); it++) {
          KeybindData* keybindData = it->second;

          //Queue force release of key if input is blocked and override mode is force release
          if (isInputBlocked && keybindData->overrideMode == AMMONITE_FORCE_RELEASE) {
            forceReleaseKeys.push_back(keybindData);
            continue;
          }

          //Run callback if it's not a toggle keybind
          if (!(keybindData->toggle)) {
            keybindData->callback(keybindData->keycode, GLFW_REPEAT, keybindData->userPtr);
          }
        }

        //Force release queued keys from last loop
        for (auto it = forceReleaseKeys.begin(); it != forceReleaseKeys.end(); it++) {
          KeybindData* keybindData = *it;
          if (!(keybindData->toggle)) {
            keybindData->callback(keybindData->keycode, GLFW_RELEASE, keybindData->userPtr);
          }
          heldKeybindMap.erase(keybindData->keycode);
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
      void setupInputCallback(GLFWwindow* windowPtr) {
        glfwSetKeyCallback(windowPtr, keyCallbackHandler);
      }

      void setInputBlock(bool inputBlocked) {
        isInputBlocked = inputBlocked;
      }

      bool getInputBlock() {
        return isInputBlocked;
      }

      int registerRawKeybind(int keycode, AmmoniteEnum overrideMode, bool toggle,
                             void(*callback)(int, int, void*), void* userPtr) {
        //Check key isn't already bound
        if (keybindMap.contains(keycode)) {
          ammoniteInternalDebug << "Keycode '" << keycode << "' already registered" << std::endl;
          return -1;
        }

        //Validate override mode
        if (overrideMode < AMMONITE_ALLOW_OVERRIDE || overrideMode > AMMONITE_RESPECT_BLOCK) {
          ammoniteInternalDebug << "Invalid override mode passed" << std::endl;
          return -1;
        }

        //Bundle keybind data and add to tracker
        KeybindData keybindData = {
          keycode, overrideMode, toggle, callback, userPtr
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
