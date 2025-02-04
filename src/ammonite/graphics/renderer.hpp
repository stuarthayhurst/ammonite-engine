#ifndef RENDERER
#define RENDERER

#include <cstdint>
#include <string>

extern "C" {
  #include <GL/glew.h>
}

#include "../enums.hpp"
#include "../internal.hpp"
#include "../types.hpp"

namespace ammonite {
  namespace renderer {
    namespace setup {
      namespace AMMONITE_INTERNAL internal {
        bool checkGPUCapabilities(unsigned int* failureCount);
        bool createShaders(std::string shaderPath);
        void setupOpenGLObjects();
        void deleteShaders();
        void destroyOpenGLObjects();
        void deleteModelCache();
      }
    }

    namespace settings {
      namespace post {
        namespace AMMONITE_INTERNAL internal {
          bool* getFocalDepthEnabledPtr();
          float* getFocalDepthPtr();
          float* getBlurStrengthPtr();
        }
      }

      namespace AMMONITE_INTERNAL internal {
        float* getFrameLimitPtr();
        unsigned int* getShadowResPtr();
        float* getRenderResMultiplierPtr();
        unsigned int* getAntialiasingSamplesPtr();
        float* getRenderFarPlanePtr();
        float* getShadowFarPlanePtr();
        bool* getGammaCorrectionPtr();
      }
    }

    namespace AMMONITE_INTERNAL internal {
      void internalDrawFrame();
      void internalDrawLoadingScreen(AmmoniteId loadingScreenId);

      void prepareScreen(GLuint framebufferId, unsigned int width,
                         unsigned int height, bool depthTest);
      void setWireframe(bool enabled);
    }


    //Exported by the engine
    namespace setup {
      bool setupRenderer(std::string shaderPath);
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

    uintmax_t getTotalFrames();
    double getFrameTime();
    void drawFrame();
  }
}

#endif
