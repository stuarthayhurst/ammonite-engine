#ifndef SETTINGS
#define SETTINGS

namespace ammonite {
  namespace settings {
    namespace controls {
      void setMovementSpeed(float newMovementSpeed);
      void setMouseSpeed(float newMouseSpeed);
      void setZoomSpeed(float newZoomSpeed);

      float getMovementSpeed();
      float getMouseSpeed();
      float getZoomSpeed();
    }

    namespace graphics {
      void setVsync(bool enabled);
      bool getVsync();
      void setFrameLimit(float frameTarget);
      float getFrameLimit();
      void setShadowRes(int shadowRes);
      int getShadowRes();
    }
  }
}

#endif
