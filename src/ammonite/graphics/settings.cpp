#include "renderer.hpp"

namespace ammonite {
  namespace renderer {
    namespace settings {
      namespace {
        struct PostSettings {
          bool focalDepthEnabled = false;
          float focalDepth = 0.0f;
          float blurStrength = 1.0f;
        } postSettings;

        struct GraphicsSettings {
          bool vsyncEnabled = true;
          float frameLimit = 0.0f;
          unsigned int shadowRes = 1024;
          float renderResMultiplier = 1.0f;
          unsigned int antialiasingSamples = 0;
          float renderFarPlane = 100.0f;
          float shadowFarPlane = 25.0f;
          bool gammaCorrection = false;
        } graphicsSettings;
      }

      namespace post {
        void setFocalDepthEnabled(bool enabled) {
          postSettings.focalDepthEnabled = enabled;
        }

        bool getFocalDepthEnabled() {
          return postSettings.focalDepthEnabled;
        }

        void setFocalDepth(float depth) {
          postSettings.focalDepth = depth;
        }

        float getFocalDepth() {
          return postSettings.focalDepth;
        }

        void setBlurStrength(float strength) {
          postSettings.blurStrength = strength;
        }

        float getBlurStrength() {
          return postSettings.blurStrength;
        }
      }

      void setVsync(bool enabled) {
        graphicsSettings.vsyncEnabled = enabled;
      }

      bool getVsync() {
        return graphicsSettings.vsyncEnabled;
      }

      void setFrameLimit(float frameLimit) {
        //Override with 0 if given a negative
        graphicsSettings.frameLimit = frameLimit > 0.0 ? frameLimit : 0;
      }

      float getFrameLimit() {
        return graphicsSettings.frameLimit;
      }

      void setShadowRes(unsigned int shadowRes) {
        graphicsSettings.shadowRes = shadowRes;
      }

      unsigned int getShadowRes() {
        return graphicsSettings.shadowRes;
      }

      void setRenderResMultiplier(float renderResMultiplier) {
        graphicsSettings.renderResMultiplier = renderResMultiplier;
      }

      float getRenderResMultiplier() {
        return graphicsSettings.renderResMultiplier;
      }

      void setAntialiasingSamples(unsigned int samples) {
        graphicsSettings.antialiasingSamples = samples;
      }

      unsigned int getAntialiasingSamples() {
        return graphicsSettings.antialiasingSamples;
      }

      void setRenderFarPlane(float renderFarPlane) {
        graphicsSettings.renderFarPlane = renderFarPlane;
      }

      float getRenderFarPlane() {
        return graphicsSettings.renderFarPlane;
      }

      void setShadowFarPlane(float shadowFarPlane) {
        graphicsSettings.shadowFarPlane = shadowFarPlane;
      }

      float getShadowFarPlane() {
        return graphicsSettings.shadowFarPlane;
      }

      void setGammaCorrection(bool gammaCorrection) {
        graphicsSettings.gammaCorrection = gammaCorrection;
      }

      bool getGammaCorrection() {
        return graphicsSettings.gammaCorrection;
      }
    }
  }
}
