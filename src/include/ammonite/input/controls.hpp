#ifndef AMMONITECONTROLS
#define AMMONITECONTROLS

#include "keycodes.hpp"

#include "../visibility.hpp"

namespace AMMONITE_EXPOSED ammonite {
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

      float getRealMovementSpeed();
      float getRealMouseSpeed();
      float getRealZoomSpeed();
    }

    void setupFreeCamera(AmmoniteKeycode forwardKey, AmmoniteKeycode backKey,
                         AmmoniteKeycode upKey, AmmoniteKeycode downKey,
                         AmmoniteKeycode rightKey, AmmoniteKeycode leftKey);
    void releaseFreeCamera();

    void setCameraActive(bool active);
    void setCameraActive(bool active, bool allowZoom);
    bool getCameraActive();
    bool getZoomActive();
  }
}

#endif
