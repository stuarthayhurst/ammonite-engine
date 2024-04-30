#include <GLFW/glfw3.h>

namespace ammonite {
  namespace settings {
    namespace graphics {
      namespace post {
        namespace {
          struct PostSettings {
            bool focalDepthEnabled = false;
            float focalDepth = 0.0f;
            float blurStrength = 1.0f;
          } post;
        }

        namespace internal {
          bool* getFocalDepthEnabledPtr() {
            return &post.focalDepthEnabled;
          }

          float* getFocalDepthPtr() {
            return &post.focalDepth;
          }

          float* getBlurStrengthPtr() {
            return &post.blurStrength;
          }
        }

        void setFocalDepthEnabled(bool enabled) {
          post.focalDepthEnabled = enabled;
        }

        bool getFocalDepthEnabled() {
          return post.focalDepthEnabled;
        }

        void setFocalDepth(float depth) {
          post.focalDepth = depth;
        }

        float getFocalDepth() {
          return post.focalDepth;
        }

        void setBlurStrength(float strength) {
          post.blurStrength = strength;
        }

        float getBlurStrength() {
          return post.blurStrength;
        }
      }

      namespace {
        struct GraphicsSettings {
          bool vsyncEnabled = true;
          float frameLimit = 0.0f;
          int shadowRes = 1024;
          float renderResMultiplier = 1.0f;
          int antialiasingSamples = 0;
          float renderFarPlane = 100.0f;
          float shadowFarPlane = 25.0f;
          bool gammaCorrection = false;
        } graphics;
      }

      //Exposed internally only
      namespace internal {
        float* getFrameLimitPtr() {
          return &graphics.frameLimit;
        }

        int* getShadowResPtr() {
          return &graphics.shadowRes;
        }

        float* getRenderResMultiplierPtr() {
          return &graphics.renderResMultiplier;
        }

        int* getAntialiasingSamplesPtr() {
          return &graphics.antialiasingSamples;
        }

        float* getRenderFarPlanePtr() {
          return &graphics.renderFarPlane;
        }

        float* getShadowFarPlanePtr() {
          return &graphics.shadowFarPlane;
        }

        bool* getGammaCorrectionPtr() {
          return &graphics.gammaCorrection;
        }
      }

      void setVsync(bool enabled) {
        glfwSwapInterval(int(enabled));
        graphics.vsyncEnabled = enabled;
      }

      bool getVsync() {
        return graphics.vsyncEnabled;
      }

      void setFrameLimit(float frameLimit) {
        //Override with 0 if given a negative
        graphics.frameLimit = frameLimit > 0.0 ? frameLimit : 0;
      }

      float getFrameLimit() {
        return graphics.frameLimit;
      }

      void setShadowRes(int shadowRes) {
        graphics.shadowRes = shadowRes;
      }

      int getShadowRes() {
        return graphics.shadowRes;
      }

      void setRenderResMultiplier(float renderResMultiplier) {
        graphics.renderResMultiplier = renderResMultiplier;
      }

      float getRenderResMultiplier() {
        return graphics.renderResMultiplier;
      }

      void setAntialiasingSamples(int samples) {
        graphics.antialiasingSamples = samples;
      }

      int getAntialiasingSamples() {
        return graphics.antialiasingSamples;
      }

      void setRenderFarPlane(float renderFarPlane) {
        graphics.renderFarPlane = renderFarPlane;
      }

      float getRenderFarPlane() {
        return graphics.renderFarPlane;
      }

      void setShadowFarPlane(float shadowFarPlane) {
        graphics.shadowFarPlane = shadowFarPlane;
      }

      float getShadowFarPlane() {
        return graphics.shadowFarPlane;
      }

      void setGammaCorrection(bool gammaCorrection) {
        graphics.gammaCorrection = gammaCorrection;
      }

      bool getGammaCorrection() {
        return graphics.gammaCorrection;
      }
    }
  }
}
