#ifndef CONTROLS
#define CONTROLS

namespace ammonite {
  namespace utils {
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

      void setupFreeCamera(int forwardKey, int backKey, int upKey, int downKey,
                           int rightKey, int leftKey);
      void releaseFreeCamera();

      void setCameraActive(bool active);
      bool getCameraActive();
    }
  }
}

#endif
