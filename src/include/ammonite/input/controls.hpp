#ifndef AMMONITECONTROLS
#define AMMONITECONTROLS

#include "keycodes.hpp"

namespace ammonite {
  namespace controls {
    namespace settings {
      void setMovementSpeed(float newMovementSpeed);
      void setMouseSpeed(float newMouseSpeed);
      void setZoomSpeed(float newZoomSpeed);
      void setFovLimit(float newFovLimit);

      float getMovementSpeed();
      float getMouseSpeed();
      float getZoomSpeed();
      float getFovLimit();
    }

    void setupFreeCamera(AmmoniteKeycode forwardKey, AmmoniteKeycode backKey,
                         AmmoniteKeycode upKey, AmmoniteKeycode downKey,
                         AmmoniteKeycode rightKey, AmmoniteKeycode leftKey);
    void releaseFreeCamera();

    void setCameraActive(bool active);
    bool getCameraActive();
  }
}

#endif
