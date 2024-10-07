#ifndef RENDERER
#define RENDERER

namespace ammonite {
  namespace renderer {
    namespace setup {
      void setupRenderer(std::string shaderPath, bool* externalSuccess);
      void destroyRenderer();
    }

    long long getTotalFrames();
    double getFrameTime();

    void drawFrame();

    namespace settings {
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
      void setRenderFarPlane(float renderFarPlane);
      void setShadowFarPlane(float shadowFarPlane);
      void setGammaCorrection(bool gammaCorrection);

      bool getVsync();
      float getFrameLimit();
      int getShadowRes();
      float setRenderResMultiplier();
      int setAntialiasingSamples();
      float getRenderFarPlane();
      float getShadowFarPlane();
      bool getGammaCorrection();
    }
  }
}

#endif
