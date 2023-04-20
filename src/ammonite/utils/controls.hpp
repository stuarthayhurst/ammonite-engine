#ifndef CONTROLS
#define CONTROLS

namespace ammonite {
  namespace utils {
    namespace controls {
      void setCameraActive(bool active);
      void setControlsActive(bool active);
      void setInputFocus(bool active);

      bool getCameraActive();
      bool getControlsActive();
      bool getInputFocus();

      void setupControls();
      bool shouldWindowClose();
      void processInput();
    }
  }
}

#endif
