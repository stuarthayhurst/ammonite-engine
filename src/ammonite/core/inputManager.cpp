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
          std::vector<int> keycodes;
          AmmoniteEnum overrideMode;
          bool toggle;
          void(*callback)(std::vector<int>, int, void*);
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
          int keybindId;
          KeybindData* keybindData;
          KeycodeStateEnum* keycodeStateEnumPtr;
        };

        bool isInputBlocked = false;

        //Track keybind data and states
        std::map<int, KeybindData> keybindIdDataMap;
        std::map<int, KeycodeState> keycodeStateMap;
        int totalKeybinds = 0;

        //Vectors to track pressed and released keys, with a pending callback
        std::vector<KeypressInfo> pressedKeys;
        std::vector<KeypressInfo> releasedKeys;

        //Dispatch user defined code to handle keypress
        static void keyCallbackHandler(GLFWwindow*, int keycode, int, int action, int) {
          if (!keycodeStateMap.contains(keycode)) {
            ammoniteInternalDebug << "Keycode '" << keycode << "' not registered" << std::endl;
            return;
          }

          //Iterate over keybinds related to this keycode
          KeycodeState* keycodeState = &keycodeStateMap[keycode];
          auto keybindMapPtr = &keycodeState->keybindIdStateEnumMap;
          for (auto it = keybindMapPtr->begin(); it != keybindMapPtr->end(); it++) {
            int keybindId = it->first;
            KeycodeStateEnum* keycodeStateEnumPtr = &it->second;
            KeybindData* keybindData = &keybindIdDataMap[keybindId];

            //Handle input block and override modes
            if (isInputBlocked) {
              switch (keybindData->overrideMode) {
              case AMMONITE_ALLOW_OVERRIDE: //Allow keypress
                break;

              case AMMONITE_ALLOW_RELEASE: //Allow keypress if released, and was previously tracked
                if (action == GLFW_RELEASE && *keycodeStateEnumPtr == AMMONITE_HELD) {
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
              keycode, keybindId, keybindData, keycodeStateEnumPtr
            };

            //Track new state for the keybind
            if (action == GLFW_PRESS) {
              //Track newly pressed keys
              if (*keycodeStateEnumPtr != AMMONITE_HELD) {
                pressedKeys.push_back(keypressInfo);
              } else {
                ammoniteInternalDebug << "Keycode '" << keycode << "' already held" << std::endl;
              }
            } else if (action == GLFW_RELEASE) {
              //Track released keys
              if (*keycodeStateEnumPtr == AMMONITE_HELD) {
                releasedKeys.push_back(keypressInfo);
              } else {
                ammoniteInternalDebug << "Keycode '" << keycode << "' wasn't held" << std::endl;
              }
            }
          }
        }
      }

      //Use tracked states to update saved states and run callbacks
      void runCallbacks() {
        //Update state and run callbacks for newly released keybinds
        for (auto it = releasedKeys.begin(); it != releasedKeys.end(); it++) {
          KeypressInfo keypressInfo = *it;
          KeybindData* keybindData = keypressInfo.keybindData;
          int keybindId = keypressInfo.keybindId;

          //Skip callback if it's a toggle
          if (keybindData->toggle) {
            *(keypressInfo.keycodeStateEnumPtr) = AMMONITE_RELEASED;
            continue;
          }

          //Check if any other key of the related keybind is already released
          bool runCallback = true;
          std::vector<int>* keycodes = &keybindData->keycodes;
          for (unsigned int i = 0; i < keycodes->size(); i++) {
            KeycodeState* keycodeState = &keycodeStateMap[(*keycodes)[i]];

            //If a key is already released, break and move to next key
            if (keycodeState->keybindIdStateEnumMap[keybindId] != AMMONITE_HELD) {
              runCallback = false;
              break;
            }
          }

          //Update the state and possibly run callback
          *(keypressInfo.keycodeStateEnumPtr) = AMMONITE_RELEASED;
          if (runCallback) {
            keybindData->callback(keybindData->keycodes, GLFW_RELEASE, keybindData->userPtr);
          }
        }
        releasedKeys.clear();

        //Force releasing keybinds requires the ID and whether all keycodes were held
        struct ForceReleaseInfo {
          int keybindId;
          KeybindData* keybindData;
          bool runReleaseCallback;
        };

        //Run callbacks for held keybinds, queue any keys to be force released
        std::vector<ForceReleaseInfo> forceReleaseKeybinds;
        for (auto it = keybindIdDataMap.begin(); it != keybindIdDataMap.end(); it++) {
          int keybindId = it->first;
          KeybindData* keybindData = &it->second;


          //Check all keys are held
          bool runCallback = true;
          std::vector<int>* keycodes = &keybindData->keycodes;
          for (unsigned int i = 0; i < keycodes->size(); i++) {
            KeycodeState* keycodeState = &keycodeStateMap[(*keycodes)[i]];

            if (keycodeState->keybindIdStateEnumMap[keybindId] != AMMONITE_HELD) {
              runCallback = false;
              break;
            }
          }

          //Queue force release of keycodes if input is blocked and override mode is force release
          if (isInputBlocked && keybindData->overrideMode == AMMONITE_FORCE_RELEASE) {
            forceReleaseKeybinds.push_back({
              keybindId, keybindData, runCallback
            });
            continue;
          }

          //Run callback if all keys are held and it's not a toggle keybind
          if (runCallback && !(keybindData->toggle)) {
            keybindData->callback(keybindData->keycodes, GLFW_REPEAT, keybindData->userPtr);
          }
        }

        //Force release queued keybinds from last loop, run callback if all were held
        for (auto it = forceReleaseKeybinds.begin(); it != forceReleaseKeybinds.end(); it++) {
          ForceReleaseInfo forceReleaseInfo = *it;
          KeybindData* keybindData = forceReleaseInfo.keybindData;
          int keybindId = forceReleaseInfo.keybindId;

          if (forceReleaseInfo.runReleaseCallback && !(keybindData->toggle)) {
            keybindData->callback(keybindData->keycodes, GLFW_RELEASE, keybindData->userPtr);
          }

          //Force release the keys within the keybind
          std::vector<int>* keycodes = &keybindData->keycodes;
          for (unsigned int i = 0; i < keycodes->size(); i++) {
            KeycodeState* keycodeState = &keycodeStateMap[(*keycodes)[i]];
            keycodeState->keybindIdStateEnumMap[keybindId] = AMMONITE_RELEASED;
          }
        }

        //Update state and run callbacks for pressed keybinds
        for (auto it = pressedKeys.begin(); it != pressedKeys.end(); it++) {
          KeypressInfo keypressInfo = *it;
          KeybindData* keybindData = keypressInfo.keybindData;
          int keybindId = keypressInfo.keybindId;

          *keypressInfo.keycodeStateEnumPtr = AMMONITE_HELD;

          //Check all keycodes of the keybind are held
          bool runCallback = true;
          std::vector<int>* keycodes = &keybindData->keycodes;
          for (unsigned int i = 0; i < keycodes->size(); i++) {
            KeycodeState* keycodeState = &keycodeStateMap[(*keycodes)[i]];

            //If a key isn't held, break and move to next key
            if (keycodeState->keybindIdStateEnumMap[keybindId] != AMMONITE_HELD) {
              runCallback = false;
              break;
            }
          }

          if (runCallback) {
            keybindData->callback(keybindData->keycodes, GLFW_PRESS, keybindData->userPtr);
          }
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

      int registerRawKeybind(int keycodes[], int count, AmmoniteEnum overrideMode,
                             bool toggle, void(*callback)(std::vector<int>, int, void*), void* userPtr) {
        //Validate override mode
        if (overrideMode < AMMONITE_ALLOW_OVERRIDE || overrideMode > AMMONITE_RESPECT_BLOCK) {
          ammoniteInternalDebug << "Invalid override mode passed" << std::endl;
          return -1;
        }

        int keybindId = ++totalKeybinds;
        std::vector<int> keycodeVec;

        //Start tracking keycode states
        for (int i = 0; i < count; i++) {
          int keycode = keycodes[i];
          keycodeVec.push_back(keycode);

          if (!keycodeStateMap.contains(keycode)) {
            KeycodeState keycodeState;
            keycodeState.refCount = 0;
            keycodeState.keybindIdStateEnumMap[keybindId] = AMMONITE_RELEASED;

            keycodeStateMap[keycode] = keycodeState;
          }
          keycodeStateMap[keycode].refCount++;

          //Set initial key state
          keycodeStateMap[keycode].keybindIdStateEnumMap[keybindId] = AMMONITE_RELEASED;
        }

        //Bundle keybind data and add to tracker
        KeybindData keybindData = {
          keycodeVec, overrideMode, toggle, callback, userPtr
        };
        keybindIdDataMap[keybindId] = keybindData;
        return keybindId;
      }

      int unregisterKeybind(int keybindId) {
        //Exit if keybind doesn't exist
        if (!keybindIdDataMap.contains(keybindId)) {
          ammoniteInternalDebug << "Can't unregister keybind ID '" << keybindId
                                << "', not registered" << std::endl;
          return -1;
        }

        //Stop tracking keycode states
        std::vector<int>* keycodeVec = &(keybindIdDataMap[keybindId].keycodes);
        for (unsigned int i = 0; i < keycodeVec->size(); i++) {
          int keycode = (*keycodeVec)[i];
          if (keycodeStateMap.contains(keycode)) {
            //Delete keybind id's entry in nested map
            keycodeStateMap[keycode].keybindIdStateEnumMap.erase(keybindId);

            //Reduce reference counter on keycode state tracker
            if (keycodeStateMap[keycode].refCount == 1) {
              keycodeStateMap.erase(keycode);
            } else {
              keycodeStateMap[keycode].refCount--;
            }
          } else {
            ammoniteInternalDebug << "Keycode state tracking missing for '" << keycode << std::endl;
          }
        }

        //Forget the keybind data
        keybindIdDataMap.erase(keybindId);
        return 0;
      }

      bool isKeycodeRegistered(int keycode) {
        return keycodeStateMap.contains(keycode);
      }


/* TODO:
 - Needs a significant rewrite for multi-key binds
 - Unused, expose it with a wrapper from input.hpp?
*/

/*
      void moveKeybindData(int keybindId, int newKeycode) {
        KeybindData* keybindData = &keybindIdDataMap[keybindId];

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
        if (!keycodeStateMap.contains(newKeycode)) {
          KeycodeState keycodeState;
          keycodeState.refCount = 0;
          keycodeState.keybindIdStateEnumMap[keybindId] = oldKeycodeState;

          keycodeStateMap[newKeycode] = keycodeState;
        } else {
          keycodeStateMap[newKeycode].keybindIdStateEnumMap[keybindId] = oldKeycodeState;
        }
        keycodeStateMap[newKeycode].refCount++;

        //Delete old keycode's data
        keycodeStateMap[oldKeycode].keybindIdStateEnumMap.erase(keybindId);
        keycodeStateMap[oldKeycode].refCount--;
        if (keycodeStateMap[oldKeycode].refCount == 0) {
          keycodeStateMap.erase(oldKeycode);
        }
      }
      */
    }
  }
}
