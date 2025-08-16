#include <iostream>
#include <unordered_map>
#include <vector>

#include "input.hpp"
#include "keycodes.hpp"

#include "../utils/debug.hpp"
#include "../utils/id.hpp"

/*
 - This file implements keybind and keycode state tracking
 - Interest in keycodes is registered through keybinds
   - These keycodes have their states tracked, to determine when to run keybind callbacks
*/

namespace ammonite {
  namespace input {
    namespace internal {
      namespace {
        struct KeybindData {
          std::vector<AmmoniteKeycode> keycodes;
          AmmoniteReleaseEnum overrideMode;
          bool toggle;
          AmmoniteKeyCallback callback;
          void* userPtr;
          KeyStateEnum lastState = AMMONITE_RELEASED;
          bool debugLogAllowed = true;
        };

        struct KeycodeData {
          unsigned int refCount = 0;
          KeyStateEnum state;
        };

        bool isInputBlocked = false;

        std::unordered_map<AmmoniteId, KeybindData> keybindIdDataMap;
        std::unordered_map<AmmoniteKeycode, KeycodeData> keycodeStateMap;
        AmmoniteId lastKeybindId = 0;

        AmmoniteKeyCallback anykeyCallback = nullptr;
        void* anykeyCallbackData = nullptr;
      }

      //Internal functions
      namespace {
        //Track states for an array of keycodes
        void registerKeycodes(const AmmoniteKeycode keycodes[], int count) {
          for (int i = 0; i < count; i++) {
            if (!keycodeStateMap.contains(keycodes[i])) {
              keycodeStateMap[keycodes[i]].state = getKeyState(keycodes[i]);
            }

            keycodeStateMap[keycodes[i]].refCount++;
          }
        }

        //Stop tracking states for an array of keycodes
        void unregisterKeycodes(const AmmoniteKeycode keycodes[], int count) {
          for (int i = 0; i < count; i++) {
            keycodeStateMap[keycodes[i]].refCount--;
            if (keycodeStateMap[keycodes[i]].refCount == 0) {
              ammoniteInternalDebug << "Deleted storage for keycode state (" << keycodes[i] \
                                    << ")" << std::endl;
              keycodeStateMap.erase(keycodes[i]);
            }
          }
        }

        void runAnykeyCallback() {
          if (anykeyCallback == nullptr) {
            return;
          }

          const std::vector<KeycodeStatePair>* updatedKeysPtr = getUpdatedKeys();
          for (const auto& updatedKey : *updatedKeysPtr) {
            anykeyCallback({updatedKey.keycode}, updatedKey.state, anykeyCallbackData);
          }

          clearUpdatedKeys();
        }
      }

      KeyStateEnum* getKeycodeStatePtr(AmmoniteKeycode keycode) {
        if (keycodeStateMap.contains(keycode)) {
          return &keycodeStateMap[keycode].state;
        }

        return nullptr;
      }

      void setKeyInputBlock(bool inputBlocked) {
        isInputBlocked = inputBlocked;
      }

      bool getKeyInputBlock() {
        return isInputBlocked;
      }

      //Used tracked states and keybind settings to run callbacks and update keybind states
      void runCallbacks() {
        //Check new state of each keybind and potentially run its callback
        for (auto& keybindEntry : keybindIdDataMap) {
          auto& keybindData = keybindEntry.second;
          //Determine keybind state
          KeyStateEnum keybindState = AMMONITE_PRESSED;
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
              if (keybindState == AMMONITE_PRESSED) {
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
            }
          } else {
            keybindData.debugLogAllowed = true;
          }

          //Run the callback if the keybind is down or was just released
          bool runCallback = false;
          if (keybindState == AMMONITE_PRESSED ||
              (keybindData.lastState == AMMONITE_PRESSED && keybindState == AMMONITE_RELEASED)) {
            runCallback = true;
          }

          //Filter toggle keybinds if the keybind was already pressed
          if (runCallback && keybindData.toggle) {
            if (keybindData.lastState == AMMONITE_PRESSED) {
              runCallback = false;
            }
          }

          //Determine reported keybind state and run callback
          if (allowCallback && runCallback) {
            //Determine which action triggered the callback
            KeyStateEnum userState = keybindState;
            if (keybindData.lastState == keybindState) {
              userState = AMMONITE_REPEAT;
            }

            if (userState == AMMONITE_PRESSED) {
              ammoniteInternalDebug << "Keybind '" << keybindEntry.first \
                                    << "' pressed" << std::endl;
            } else if (userState == AMMONITE_RELEASED) {
              ammoniteInternalDebug << "Keybind '" << keybindEntry.first \
                                    << "' released" << std::endl;
            }

            //Run the callback
            keybindData.callback(keybindData.keycodes, userState, keybindData.userPtr);
          }

          //Update keybind's previous state
          if (allowStateChange) {
            keybindData.lastState = keybindState;
          }
        }

        //Handle anykey callback
        runAnykeyCallback();
      }

      //Register a keybind
      AmmoniteId registerRawKeybind(const AmmoniteKeycode keycodes[], int count,
                                    AmmoniteReleaseEnum overrideMode, bool toggle,
                                    AmmoniteKeyCallback callback, void* userPtr) {
        //Validate override mode
        if (overrideMode < AMMONITE_ALLOW_OVERRIDE || overrideMode > AMMONITE_RESPECT_BLOCK) {
          ammoniteInternalDebug << "Invalid override mode passed" << std::endl;
          return 0;
        }

        //Initialise and / or increase reference counter for each keycode tracked
        registerKeycodes(keycodes, count);

        //Generate an ID for the keybind and register it
        const AmmoniteId keybindId = utils::internal::setNextId(&lastKeybindId, keybindIdDataMap);
        const std::vector<AmmoniteKeycode> keycodeVec = {keycodes, keycodes + count};
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
      bool isKeycodeRegistered(const AmmoniteKeycode keycodes[], int count) {
        //Check keycodes against registered keybinds
        for (const auto& keybindData : keybindIdDataMap) {
          //Move onto the next keybind if any keycode isn't found in the keybind
          bool matched = true;
          for (int i = 0; i < count; i++) {
            bool found = false;

            //Search keybind's keycodes for current keycode
            const std::vector<AmmoniteKeycode>& keybindKeycodes = keybindData.second.keycodes;
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

      bool changeKeybindKeycodes(AmmoniteId keybindId,
                                 const AmmoniteKeycode newKeycodes[], int count) {
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

      void setAnykeyCallback(AmmoniteKeyCallback callback, void* userPtr) {
        anykeyCallback = callback;
        anykeyCallbackData = userPtr;
      }
    }
  }
}
