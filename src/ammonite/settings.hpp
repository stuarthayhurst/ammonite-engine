#ifndef SETTINGS
#define SETTINGS

namespace ammonite {
  namespace settings {
    namespace controls {
      void setMovementSpeed(float newMovementSpeed);
      void setMouseSpeed(float newMouseSpeed);
      void setZoomSpeed(float newZoomSpeed);
      void setFovLimit(float newFovLimit);

      float getMovementSpeed();
      float getMouseSpeed();
      float getZoomSpeed();
      float getFovLimit();
    }

    namespace graphics {
      void setVsync(bool enabled);
      void setFrameLimit(float frameTarget);
      void setShadowRes(int shadowRes);

      bool getVsync();
      float getFrameLimit();
      int getShadowRes();
    }
  }

  namespace settings {
    namespace runtime {
      int getWidth();
      int getHeight();
    }
  }
}

#endif
