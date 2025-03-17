#include <iostream>
#include <map>
#include <vector>

extern "C" {
  #include <GLFW/glfw3.h>
}

#include "../input.hpp"

#include "../enums.hpp"
#include "../types.hpp"
#include "../utils/debug.hpp"
#include "../utils/id.hpp"

namespace ammonite {
  namespace input {
    namespace internal {
      namespace {
        struct KeybindData {
          std::vector<int> keycodes;
          AmmoniteEnum overrideMode;
          bool toggle;
          AmmoniteKeyCallback callback;
          void* userPtr;
        };

        enum KeycodeStateEnum : unsigned char {
          AMMONITE_HELD,
          AMMONITE_RELEASED
        };

        struct KeycodeState {
          unsigned int refCount = 0;
          std::map<AmmoniteId, KeycodeStateEnum> keybindIdStateEnumMap;
        };

        struct KeypressInfo {
          int keycode;
          AmmoniteId keybindId;
          KeybindData* keybindData;
          KeycodeStateEnum* keycodeStateEnumPtr;
        };

        bool isInputBlocked = false;

        //Track keybind data and states
        std::map<AmmoniteId, KeybindData> keybindIdDataMap;
        std::map<int, KeycodeState> keycodeStateMap;
        AmmoniteId lastKeybindId = 0;

        //Vectors to track pressed and released keys, with a pending callback
        std::vector<KeypressInfo> pressedKeys;
        std::vector<KeypressInfo> releasedKeys;
      }

      namespace {
        //Dispatch user defined code to handle keypress
        void keyCallbackHandler(GLFWwindow*, int keycode, int, int action, int) {
          if (!keycodeStateMap.contains(keycode)) {
            ammoniteInternalDebug << "Keycode '" << keycode << "' not registered" << std::endl;
            return;
          }

          //Iterate over keybinds related to this keycode
          KeycodeState* keycodeState = &keycodeStateMap[keycode];
          auto keybindMapPtr = &keycodeState->keybindIdStateEnumMap;
          for (auto it = keybindMapPtr->begin(); it != keybindMapPtr->end(); it++) {
            AmmoniteId keybindId = it->first;
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

        //Register a keybind, use passed ID to allow forcing an ID
        AmmoniteId registerRawKeybind(const int keycodes[], int count, AmmoniteEnum overrideMode,
                                      bool toggle, AmmoniteKeyCallback callback,
                                      void* userPtr, AmmoniteId keybindId) {
          //Validate override mode
          if (overrideMode < AMMONITE_ALLOW_OVERRIDE || overrideMode > AMMONITE_RESPECT_BLOCK) {
            ammoniteInternalDebug << "Invalid override mode passed" << std::endl;
            return 0;
          }

          //Start tracking keycode states
          std::vector<int> keycodeVec;
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
      }

      //Use tracked states to update saved states and run callbacks
      void runCallbacks() {
        //Update state and run callbacks for newly released keybinds
        for (auto it = releasedKeys.begin(); it != releasedKeys.end(); it++) {
          KeypressInfo keypressInfo = *it;
          KeybindData* keybindData = keypressInfo.keybindData;
          AmmoniteId keybindId = keypressInfo.keybindId;

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
          if (runCallback && keybindData->callback != nullptr) {
            keybindData->callback(keybindData->keycodes, GLFW_RELEASE, keybindData->userPtr);
          }
        }
        releasedKeys.clear();

        //Force releasing keybinds requires the ID and whether all keycodes were held
        struct ForceReleaseInfo {
          AmmoniteId keybindId;
          KeybindData* keybindData;
          bool runReleaseCallback;
        };

        //Run callbacks for held keybinds, queue any keys to be force released
        std::vector<ForceReleaseInfo> forceReleaseKeybinds;
        for (auto it = keybindIdDataMap.begin(); it != keybindIdDataMap.end(); it++) {
          AmmoniteId keybindId = it->first;
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
          if (runCallback && keybindData->callback != nullptr && !(keybindData->toggle)) {
            keybindData->callback(keybindData->keycodes, GLFW_REPEAT, keybindData->userPtr);
          }
        }

        //Force release queued keybinds from last loop, run callback if all were held
        for (auto it = forceReleaseKeybinds.begin(); it != forceReleaseKeybinds.end(); it++) {
          ForceReleaseInfo forceReleaseInfo = *it;
          KeybindData* keybindData = forceReleaseInfo.keybindData;
          AmmoniteId keybindId = forceReleaseInfo.keybindId;

          if (forceReleaseInfo.runReleaseCallback && keybindData->callback != nullptr &&
              !(keybindData->toggle)) {
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
          AmmoniteId keybindId = keypressInfo.keybindId;

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

          if (runCallback && keybindData->callback != nullptr) {
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

      bool* getInputBlockPtr() {
        return &isInputBlocked;
      }

      AmmoniteId registerRawKeybind(int keycodes[], int count, AmmoniteEnum overrideMode,
                                    bool toggle, AmmoniteKeyCallback callback,
                                    void* userPtr) {
        //Generate an ID
        AmmoniteId keybindId = utils::internal::setNextId(&lastKeybindId, keybindIdDataMap);

        //Hand off to actual registry
        return registerRawKeybind(keycodes, count, overrideMode, toggle, callback,
                                  userPtr, keybindId);
      }

      bool unregisterKeybind(AmmoniteId keybindId) {
        //Exit if keybind doesn't exist
        if (!keybindIdDataMap.contains(keybindId)) {
          ammoniteInternalDebug << "Can't unregister keybind ID '" << keybindId
                                << "', not registered" << std::endl;
          return false;
        }

        //Stop tracking keycode states
        std::vector<int>* keycodeVec = &(keybindIdDataMap[keybindId].keycodes);
        for (unsigned int i = 0; i < keycodeVec->size(); i++) {
          int keycode = (*keycodeVec)[i];
          if (keycodeStateMap.contains(keycode)) {
            //Delete keybind id's entry in nested map
            keycodeStateMap[keycode].keybindIdStateEnumMap.erase(keybindId);

            //Reduce reference counter on keycode state tracker
            keycodeStateMap[keycode].refCount--;
            if (keycodeStateMap[keycode].refCount == 0) {
              keycodeStateMap.erase(keycode);
            }
          } else {
            ammoniteInternalDebug << "Keycode state tracking missing for '" << keycode << std::endl;
          }
        }

        //Forget the keybind data
        keybindIdDataMap.erase(keybindId);
        return true;
      }

      //Return true if all keys are at least part of the same combo
      bool isKeycodeRegistered(int keycodes[], int count) {
        if (count == 0) {
          return false;
        }

        //Fill initial list of potential IDs
        std::vector<AmmoniteId> potentialIds;
        auto idStateEnumMap = keycodeStateMap[keycodes[0]].keybindIdStateEnumMap;
        for (auto it = idStateEnumMap.begin(); it != idStateEnumMap.end(); it++) {
          potentialIds.push_back(it->first);
        }

        //Iterate over each keycode, and at each step remove IDs that weren't at the last step
        for (int i = 1; i < count; i++) {
          if (!keycodeStateMap.contains(keycodes[i])) {
            return false;
          }

          //Remove elements from vector if they're not in the map
          std::map<AmmoniteId, KeycodeStateEnum>* idStateEnumMapPtr =
            &keycodeStateMap[keycodes[i]].keybindIdStateEnumMap;
          std::erase_if(potentialIds, [idStateEnumMapPtr](AmmoniteId x) {
            return !idStateEnumMapPtr->contains(x);
          });
        }

        //Return true if any IDs remain
        return !potentialIds.empty();
      }

      bool changeKeybindKeycodes(AmmoniteId keybindId, int newKeycodes[], int count) {
        //Check the keybind exists
        if (!keybindIdDataMap.contains(keybindId)) {
          ammoniteInternalDebug << "Can't change keycodes for keybind ID '" << keybindId
                                << "', not registered" << std::endl;
          return false;
        }

        //Save details about the keybind
        KeybindData* keybindData = &keybindIdDataMap[keybindId];
        AmmoniteEnum overrideMode = keybindData->overrideMode;
        bool toggle = keybindData->toggle;
        AmmoniteKeyCallback callback = keybindData->callback;
        void* userPtr = keybindData->userPtr;

        //Unregister the keybind
        unregisterKeybind(keybindId);

        //Register the keybind under the same ID, with new keycodes
        registerRawKeybind(newKeycodes, count, overrideMode, toggle, callback,
                           userPtr, keybindId);

        return true;
      }
    }
  }
}
