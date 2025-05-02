#include <iostream>
#include <unordered_map>
#include <vector>

extern "C" {
  #include <GLFW/glfw3.h>
}

#include "input.hpp"

#include "../enums.hpp"
#include "../types.hpp"
#include "../utils/debug.hpp"
#include "../utils/id.hpp"

namespace ammonite {
  namespace input {
    namespace internal {
      namespace {
        enum KeycodeStateEnum : unsigned char {
          AMMONITE_HELD,
          AMMONITE_RELEASED
        };

        struct KeybindData {
          std::vector<int> keycodes;
          AmmoniteEnum overrideMode;
          bool toggle;
          AmmoniteKeyCallback callback;
          void* userPtr;
          KeycodeStateEnum lastState = AMMONITE_RELEASED;
          bool debugLogAllowed = true;
        };

        struct KeycodeData {
          unsigned int refCount = 0;
          KeycodeStateEnum state;
        };

        GLFWwindow* windowPtr = nullptr;

        bool isInputBlocked = false;

        std::unordered_map<AmmoniteId, KeybindData> keybindIdDataMap;
        std::unordered_map<int, KeycodeData> keycodeStateMap;
        AmmoniteId lastKeybindId = 0;
      }

      //Window implementation specific internal functions
      namespace {
        //Update the states of tracked keys on input
        void keyCallbackHandler(GLFWwindow*, int keycode, int, int action, int) {
          //Filter out unmapped keys
          if (!keycodeStateMap.contains(keycode)) {
            ammoniteInternalDebug << "Keycode '" << keycode << "' not registered" << std::endl;
            return;
          }

          //Track new state for the keycode
          if (action == GLFW_PRESS) {
            if (keycodeStateMap[keycode].state == AMMONITE_HELD) {
              ammoniteInternalDebug << "Keycode '" << keycode << "' already held" << std::endl;
            }

            keycodeStateMap[keycode].state = AMMONITE_HELD;
          } else if (action == GLFW_RELEASE) {
            if (keycodeStateMap[keycode].state == AMMONITE_RELEASED) {
              ammoniteInternalDebug << "Keycode '" << keycode << "' wasn't held" << std::endl;
            }

            keycodeStateMap[keycode].state = AMMONITE_RELEASED;
          }
        }

        KeycodeStateEnum getKeyState(int keycode) {
          //Treat key as unpressed if the window isn't ready yet
          if (windowPtr == nullptr) {
            return AMMONITE_RELEASED;
          }

          if (glfwGetKey(windowPtr, keycode) == GLFW_RELEASE) {
            return AMMONITE_RELEASED;
          }

          return AMMONITE_HELD;
        }
      }

      //Generic internal functions
      namespace {
        //Track states for an array of keycodes
        void registerKeycodes(const int keycodes[], int count) {
          for (int i = 0; i < count; i++) {
            if (!keycodeStateMap.contains(keycodes[i])) {
              keycodeStateMap[keycodes[i]].state = getKeyState(keycodes[i]);
            }

            keycodeStateMap[keycodes[i]].refCount++;
          }
        }

        //Stop tracking states for an array of keycodes
        void unregisterKeycodes(const int keycodes[], int count) {
          for (int i = 0; i < count; i++) {
            keycodeStateMap[keycodes[i]].refCount--;
            if (keycodeStateMap[keycodes[i]].refCount == 0) {
              keycodeStateMap.erase(keycodes[i]);
            }
          }
        }
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

      //Used tracked states and keybind settings to run callbacks and update keybind states
      void runCallbacks() {
        //Check new state of each keybind and potentially run its callback
        for (auto& keybindEntry : keybindIdDataMap) {
          auto& keybindData = keybindEntry.second;
          //Determine keybind state
          KeycodeStateEnum keybindState = AMMONITE_HELD;
          for (unsigned int i = 0; i < keybindData.keycodes.size(); i++) {
            if (keycodeStateMap[keybindData.keycodes[i]].state == AMMONITE_RELEASED) {
              keybindState = AMMONITE_RELEASED;
              break;
            }
          }

          //Handle input blocking and special release modes
          bool allowCallback = true;
          bool allowStateChange = true;
          if (isInputBlocked) {
            switch (keybindData.overrideMode) {
            case AMMONITE_ALLOW_OVERRIDE: //Always allow keybinds
              break;

            case AMMONITE_ALLOW_RELEASE: //Block newly pressed keybinds
              if (keybindData.lastState == AMMONITE_RELEASED) {
                allowCallback = false;
                allowStateChange = false;

                //Log blocked keybind once
                if (keybindData.debugLogAllowed) {
                  keybindData.debugLogAllowed = false;
                  ammoniteInternalDebug << "Keybind '" << keybindEntry.first \
                                        << "' blocked" << std::endl;
                }
              }
              break;

            case AMMONITE_FORCE_RELEASE: //Force keybind to be released, including the state
              //Log blocked keybind once per press
              if (keybindState == AMMONITE_HELD) {
                if (keybindData.debugLogAllowed) {
                  keybindData.debugLogAllowed = false;
                  ammoniteInternalDebug << "Keybind '" << keybindEntry.first \
                                        << "' blocked" << std::endl;
                }
              } else {
                keybindData.debugLogAllowed = true;
              }

              keybindState = AMMONITE_RELEASED;
              break;

            case AMMONITE_RESPECT_BLOCK: //Keep running keybind with the last state
              keybindState = keybindData.lastState;
              allowStateChange = false;
              break;

            default: //Unhandled override, send debug and carry on
              if (keybindData.debugLogAllowed) {
                keybindData.debugLogAllowed = false;
                ammoniteInternalDebug << "Keybind '" << keybindEntry.first \
                                      << "' has unexpected override mode" << std::endl;
              }
              break;
            }
          } else {
            keybindData.debugLogAllowed = true;
          }

          //Run the callback if the keybind is down or was just released
          bool runCallback = false;
          if (keybindState == AMMONITE_HELD ||
              (keybindData.lastState == AMMONITE_HELD && keybindState == AMMONITE_RELEASED)) {
            runCallback = true;
          }

          //Filter toggle keybinds if the keybind was already pressed
          if (runCallback && keybindData.toggle) {
            if (keybindData.lastState == AMMONITE_HELD) {
              runCallback = false;
            }
          }

          //Determine reported keybind state and run callback
          if (allowCallback && runCallback) {
            //Determine which action triggered the callback
            int userState = GLFW_RELEASE;
            if (keybindData.lastState == keybindState) {
              userState = GLFW_REPEAT;
            } else if (keybindState == AMMONITE_HELD) {
              userState = GLFW_PRESS;
            }

            //Run the callback
            keybindData.callback(keybindData.keycodes, userState, keybindData.userPtr);
          }

          //Update keybind's previous state
          if (allowStateChange) {
            keybindData.lastState = keybindState;
          }
        }
      }

      //Register a keybind
      AmmoniteId registerRawKeybind(const int keycodes[], int count, AmmoniteEnum overrideMode,
                                    bool toggle, AmmoniteKeyCallback callback,
                                    void* userPtr) {
        //Validate override mode
        if (overrideMode < AMMONITE_ALLOW_OVERRIDE || overrideMode > AMMONITE_RESPECT_BLOCK) {
          ammoniteInternalDebug << "Invalid override mode passed" << std::endl;
          return 0;
        }

        //Initialise and / or increase reference counter for each keycode tracked
        registerKeycodes(keycodes, count);

        //Generate an ID for the keybind and register it
        const AmmoniteId keybindId = utils::internal::setNextId(&lastKeybindId, keybindIdDataMap);
        const std::vector<int> keycodeVec = {keycodes, keycodes + count};
        keybindIdDataMap[keybindId] = {
          keycodeVec, overrideMode, toggle, callback, userPtr
        };

        return keybindId;
      }

      bool unregisterKeybind(AmmoniteId keybindId) {
        //Exit if keybind doesn't exist
        if (!keybindIdDataMap.contains(keybindId)) {
          ammoniteInternalDebug << "Can't unregister keybind ID '" << keybindId
                                << "', not registered" << std::endl;
          return false;
        }

        //Reduce reference counter for tracked keycodes, delete if 0
        const auto& keybindData = keybindIdDataMap[keybindId];
        unregisterKeycodes(keybindData.keycodes.data(), (int)keybindData.keycodes.size());

        //Forget the keybind data
        keybindIdDataMap.erase(keybindId);
        return true;
      }

      //Return true if all keys are found in the same keybind
      bool isKeycodeRegistered(const int keycodes[], int count) {
        //Check keycodes against registered keybinds
        for (const auto& keybindData : keybindIdDataMap) {
          //Move onto the next keybind if any keycode isn't found in the keybind
          bool matched = true;
          for (int i = 0; i < count; i++) {
            bool found = false;

            //Search keybind's keycodes for current keycode
            const std::vector<int>& keybindKeycodes = keybindData.second.keycodes;
            for (unsigned int j = 0; j < keybindKeycodes.size(); j++) {
              if (keybindKeycodes[j] == keycodes[i]) {
                found = true;
                break;
              }
            }

            //Give up if the current keycode wasn't found
            if (!found) {
              matched = false;
              break;
            }
          }

          if (matched) {
            return true;
          }
        }

        return false;
      }

      bool changeKeybindKeycodes(AmmoniteId keybindId, const int newKeycodes[], int count) {
        //Check the keybind exists
        if (!keybindIdDataMap.contains(keybindId)) {
          ammoniteInternalDebug << "Can't change keycodes for keybind ID '" << keybindId
                                << "', not registered" << std::endl;
          return false;
        }

        //Initialise and / or increase reference counter for each keycode tracked
        registerKeycodes(newKeycodes, count);

        //Reduce reference counter for tracked keycodes, delete if 0
        const auto& keybindData = keybindIdDataMap[keybindId];
        unregisterKeycodes(keybindData.keycodes.data(), (int)keybindData.keycodes.size());

        //Update keybind registry with new keycodes
        keybindIdDataMap[keybindId].keycodes = {newKeycodes, newKeycodes + count};
        return true;
      }
    }
  }
}
