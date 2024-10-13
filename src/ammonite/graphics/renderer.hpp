#ifndef RENDERER
#define RENDERER

namespace ammonite {
  namespace renderer {
    namespace setup {
      void setupRenderer(std::string shaderPath, bool* externalSuccess);
      void destroyRenderer();
    }

    long long unsigned int getTotalFrames();
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
      void setShadowRes(unsigned int shadowRes);
      void setRenderResMultiplier(float renderRes);
      void setAntialiasingSamples(unsigned int samples);
      void setRenderFarPlane(float renderFarPlane);
      void setShadowFarPlane(float shadowFarPlane);
      void setGammaCorrection(bool gammaCorrection);

      bool getVsync();
      float getFrameLimit();
      unsigned int getShadowRes();
      float setRenderResMultiplier();
      unsigned int setAntialiasingSamples();
      float getRenderFarPlane();
      float getShadowFarPlane();
      bool getGammaCorrection();
    }
  }
}

#endif
