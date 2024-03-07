#ifndef CONTROLS
#define CONTROLS

namespace ammonite {
  namespace utils {
    namespace controls {
      void setupFreeCamera(int forwardKey, int backKey, int upKey, int downKey,
                           int rightKey, int leftKey);
      void releaseFreeCamera();

      void setCameraActive(bool active);
      bool getCameraActive();
    }
  }
}

#endif
