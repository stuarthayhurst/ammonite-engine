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
      namespace post {
        void setFocalDepthEnabled(bool enabled);
        void setFocalDepth(float depth);
        void setBlurStrength(float strength);

        bool getFocalDepthEnabled();
        float getFocalDepth();
        float getBlurStrength();
      }

      void setVsync(bool enabled);
      void setFrameLimit(float frameTarget);
      void setShadowRes(int shadowRes);
      void setRenderResMultiplier(float renderRes);
      void setAntialiasingSamples(int samples);
      void setShadowFarPlane(float farPlane);
      void setGammaCorrection(bool gammaCorrection);

      bool getVsync();
      float getFrameLimit();
      int getShadowRes();
      float setRenderResMultiplier();
      int setAntialiasingSamples();
      float getShadowFarPlane();
      bool getGammaCorrection();
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
