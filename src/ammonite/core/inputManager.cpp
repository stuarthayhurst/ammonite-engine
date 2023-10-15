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

        enum KeycodeStateEnum {
          AMMONITE_HELD,
          AMMONITE_RELEASED
        };

        struct KeycodeState {
          int refCount = 0;
          std::map<int, KeycodeStateEnum> keybindIdStateEnumMap;
        };

        struct KeypressInfo {
          int keycode;
          int keybindData;
          KeycodeStateEnum* keycodeStateEnumPtr;
        };

        bool isInputBlocked = false;

        //Track keybind data and states
        std::map<int, KeybindData> keybindIdDataMap;
        std::map<int, KeycodeState> keycodeStateMap;
        int totalKeybinds = 0;

        //Vectors to track pressed and released keys, with a pending callback
        std::vector<KeypressInfo*> pressedKeys;
        std::vector<KeypressInfo*> releasedKeys;

        //Dispatch user defined code to handle keypress
        static void keyCallbackHandler(GLFWwindow*, int keycode, int, int action, int) {
          if (!keycodeStateMap.contains(keycode)) {
            ammoniteInternalDebug << "Keycode '" << keycode << "' not registered" << std::endl;
            return;
          }

          //Iterate over keybinds related to this keycode
          KeycodeState* keycodeState = &keycodeStateMap[keycode];
          auto keybindMapPtr = &keybindState->keybindIdStateEnumMap;
          for (auto it = keybindMapPtr->begin(); it != keybindMapPtr->end(); it++) {
            int keybindId = it->first;
            KeycodeStateEnum keycodeStateEnum = it->second;
            KeybindData* keybindData = &keybindIdDataMap[keybindId];

            //Handle input block and override modes
            if (isInputBlocked) {
              switch (keybindData->overrideMode) {
              case AMMONITE_ALLOW_OVERRIDE: //Allow keypress
                break;

              case AMMONITE_ALLOW_RELEASE: //Allow keypress if released, and was previously tracked
                if (action == GLFW_RELEASE && keycodeStateEnum == AMMONITE_HELD) {
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

            //Bundle keypress info for later handling
            KeypressInfo keypressInfo = {
              keycode, keybindData, &keycodeStateEnum
            };

            //Track new state for the keybind
            if (action == GLFW_PRESS) {
              //Track newly pressed keys
              if (keycodeStateEnum != AMMONITE_HELD) {
                pressedKeys.push_back(keypressInfo);
              } else {
                ammoniteInternalDebug << "Keycode '" << keycode << "' already held" << std::endl;
              }
            } else if (action == GLFW_RELEASE) {
              //Track released keys
              if (keycodeStateEnum == AMMONITE_HELD) {
                releasedKeys.push_back(keypressInfo);
              } else {
                ammoniteInternalDebug << "Keycode '" << keycode << "' wasn't held" << std::endl;
              }
            }
          }
        }
      }

      //Execute callbacks for anything that changed, or is held
      void runCallbacks() {
//TODO:
// - In future, this code will have to decide if more conditions are met
// - When multi-key keybinds are added, all keys must be pressed
// - Releasing any should trigger a callback
// - All keys must also be released when force release occurs

        //Run callbacks for released keys
        for (auto it = releasedKeys.begin(); it != releasedKeys.end(); it++) {
          KeypressInfo* keypressInfo = *it;
          KeybindData* keybindData = keypressInfo->keybindData;

          //Run callback if it's not a toggle
          if (!(keybindData->toggle)) {
            keybindData->callback(keybindData->keycode, GLFW_RELEASE, keybindData->userPtr);
          }
          *keypressInfo->keycodeStateEnumPtr = AMMONITE_RELEASED;
        }
        releasedKeys.clear();

        //Run callbacks for held keys
        std::vector<KeybindData*> forceReleaseKeys;
        for (auto it = keybindIdDataMap.begin(); it != keybindIdDataMap.end(); it++) {
          int keybindId = it->first;
          KeybindData* keybindData = it->second;

          //Check all keys are held
          KeycodeStateEnum keycodeState =
            keycodeStateMap[keybindData->keycode].keybindIdStateEnumMap[keybindId];
          if (keycodeState != AMMONITE_HELD) {
            continue;
          }

          //Queue force release of key if input is blocked and override mode is force release
          if (isInputBlocked && keybindData->overrideMode == AMMONITE_FORCE_RELEASE) {
            forceReleaseKeys.push_back(keybindId);
            continue;
          }

          //Run callback if it's not a toggle keybind
          if (!(keybindData->toggle)) {
            keybindData->callback(keybindData->keycode, GLFW_REPEAT, keybindData->userPtr);
          }
        }

        //Force release queued keys from last loop
        for (auto it = forceReleaseKeys.begin(); it != forceReleaseKeys.end(); it++) {
          int keybindId = *it;
          KeybindData* keybindData = keybindIdDataMap[keybindId];
          if (!(keybindData->toggle)) {
            keybindData->callback(keybindData->keycode, GLFW_RELEASE, keybindData->userPtr);
          }
          keycodeStateMap[keybindData->keycode].keybindIdStateEnumMap[keybindId] = AMMONITE_RELEASED;
        }

        //Run callbacks for pressed keys, add to held keybind map
        for (auto it = pressedKeys.begin(); it != pressedKeys.end(); it++) {
          KeypressInfo* keypressInfo = *it;
          KeybindData* keybindData = keypressInfo->keybindData;

          keybindData->callback(keybindData->keycode, GLFW_PRESS, keybindData->userPtr);
          *keypressInfo->keycodeStateEnumPtr = AMMONITE_PRESSED;
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
        //Validate override mode
        if (overrideMode < AMMONITE_ALLOW_OVERRIDE || overrideMode > AMMONITE_RESPECT_BLOCK) {
          ammoniteInternalDebug << "Invalid override mode passed" << std::endl;
          return -1;
        }

        int keybindId = ++totalKeybinds;

        //Start tracking keycode states
        if (!keycodeStateMap.contains(keycode)) {
          KeycodeState keycodeState = {
            0, {keybindId, AMMONITE_RELEASED}
          };
          keycodeStateMap[keycode] = keycodeState;
        }
        keycodeStateMap[keycode].refCount++;

        //Set initial key state
        keycodeStateMap[keycode].keybindIdStateEnumMap[keybindId] = AMMONITE_RELEASED;

        //Bundle keybind data and add to tracker
        KeybindData keybindData = {
          keycode, overrideMode, toggle, callback, userPtr
        };
        keybindIdDataMap[keybindId] = keybindData;
        return keybindId;
      }

      int unregisterKeybind(int keybindId) {
        //Exit if key wasn't bound
        if (!keybindIdDataMap.contains(keybindId)) {
          ammoniteInternalDebug << "Can't unregister keycode '" << keycode << "' not registered" << std::endl;
          return -1;
        }

        //Stop tracking keycode states
        int keycode = keybindIdDataMap[keybindId].keycode;
        if (keycodeStateMap.contains(keycode)) {
          //Delete keybind id's entry in nested map
          keycodeStateMap[keycode].keybindIdStateEnumMap.erase(keybindId)

          //Reduce reference counter on keycode state tracker
          if (keycodeStateMap[keycode].refCount == 1) {
            keycodeStateMap.erase(keycode);
          } else {
            keycodeStateMap[keycode].refCount--;
          }
        } else {
          ammoniteInternalDebug << "Keycode state tracking missing for '" << keycode << std::endl;
        }

        //Forget the keybind data
        keybindIdDataMap.erase(keybindId);
        return 0;
      }

      bool isKeycodeRegistered(int keycode) {
        return keycodeStateMap.contains(keycode);
      }

      void moveKeybindData(int keybindId, int newKeycode) {
//TODO:
// - This will need a significant rewrite when multi-key keybinds are available

        KeybindData* keybindData = keybindIdDataMap[keybindId];

        //Get old keycode info
        int oldKeycode = keybindData->keycode;
        if (oldKeycode == newKeycode) {
          return;
        }
        KeycodeStateEnum oldKeycodeState =
          keycodeStateMap[oldKeycode].keybindIdStateEnumMap[keybindId];

        //Set new keycode
        keybindData->keycode = newKeycode;

        //Update new keycode state with old keycode
        if (!keycodeStateMap.contains(keycode)) {
          KeycodeState keycodeState = {
            0, {keybindId, oldKeycodeState}
          };
          keycodeStateMap[keycode] = keycodeState;
        } else {
          keycodeStateMap[keycode].keybindIdStateEnumMap[keybindId] = oldKeycodeState;
        }
        keycodeStateMap[keycode].refCount++;

        //Delete old keycode's data
        keycodeStateMap[oldKeycode].keybindIdStateEnumMap.erase(keybindId);
        keycodeStateMap[oldKeycode].refCount--;
        if (keycodeStateMap[oldKeycode].refCount == 0) {
          keycodeStateMap.erase(oldKeycode);
        }
      }
    }
  }
}
