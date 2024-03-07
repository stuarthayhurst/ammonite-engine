#ifndef KEYBINDS
#define KEYBINDS

namespace ammonite {
  namespace utils {
    namespace controls {
      void setupFreeCamera(int forwardKey, int backKey, int upKey, int downKey,
                           int rightKey, int leftKey);
      void releaseFreeCamera();

      void setControlsActive(bool active);
      bool getControlsActive();
    }
  }
}

#endif
