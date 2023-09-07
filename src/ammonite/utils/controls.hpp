#ifndef CONTROLS
#define CONTROLS

namespace ammonite {
  namespace utils {
    namespace controls {
      void setCameraActive(bool active);
      void setControlsActive(bool active);

      bool getCameraActive();
      bool getControlsActive();

      void setupControls();
      void processInput();
    }
  }
}

#endif
