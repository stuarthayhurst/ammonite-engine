#ifndef CONTROLS
#define CONTROLS

namespace ammonite {
  namespace utils {
    namespace controls {
      void setCameraActive(bool active);
      bool getCameraActive();

      void setupControls();
      void releaseControls();
    }
  }
}

#endif
