#ifndef RENDERER
#define RENDERER

#include <cstdint>
#include <string>

#include "../enums.hpp"

namespace ammonite {
  namespace renderer {
    namespace setup {
      bool setupRenderer(const std::string& shaderPath);
      void destroyRenderer();
      void requestContextType(AmmoniteEnum contextType);
    }

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
      void setFrameLimit(float frameLimit);
      void setShadowRes(unsigned int shadowRes);
      void setRenderResMultiplier(float renderRes);
      void setAntialiasingSamples(unsigned int samples);
      void setRenderFarPlane(float renderFarPlane);
      void setShadowFarPlane(float shadowFarPlane);
      void setGammaCorrection(bool gammaCorrection);

      bool getVsync();
      float getFrameLimit();
      unsigned int getShadowRes();
      float getRenderResMultiplier();
      unsigned int getAntialiasingSamples();
      float getRenderFarPlane();
      float getShadowFarPlane();
      bool getGammaCorrection();
    }

    uintmax_t getTotalFrames();
    double getFrameTime();
    void drawFrame();
  }
}

#endif
