#include <GLFW/glfw3.h>
#include <glm/gtc/constants.hpp>

namespace ammonite {
  namespace settings {
    namespace controls {
      namespace {
        //Structure to store base speeds, multipliers and calculated speeds
        struct ControlSettings {
          struct BaseControlSettings {
            const float movementSpeed = 5.0f;
            const float mouseSpeed = 0.005f;
            const float zoomSpeed = 0.025f;
          } baseSettings;

          struct ControlMultipliers {
            float movement = 1.0f;
            float mouse = 1.0f;
            float zoom = 1.0f;
          } multipliers;

          //Default to a 120 degree field of view limit
          float fovLimit = 2 * glm::half_pi<float>() / 3;

          //Final sensitivites, exposed through "internal/runtimeSettings.hpp"
          float movementSpeed = baseSettings.movementSpeed;
          float mouseSpeed = baseSettings.mouseSpeed;
          float zoomSpeed = baseSettings.zoomSpeed;
        } controls;
      }

      //Exposed internally only
      namespace internal {
        float* getMovementSpeedPtr() {
          return &controls.movementSpeed;
        }

        float* getMouseSpeedPtr() {
          return &controls.mouseSpeed;
        }

        float* getZoomSpeedPtr() {
          return &controls.zoomSpeed;
        }

        float* getFovLimitPtr() {
          return &controls.fovLimit;
        }
      }

      void setMovementSpeed(float newMovementSpeed) {
        controls.multipliers.movement = newMovementSpeed;
        controls.movementSpeed = controls.baseSettings.movementSpeed * newMovementSpeed;
      }

      void setMouseSpeed(float newMouseSpeed) {
        controls.multipliers.mouse = newMouseSpeed;
        controls.mouseSpeed = controls.baseSettings.mouseSpeed * newMouseSpeed;
      }

      void setZoomSpeed(float newZoomSpeed) {
        controls.multipliers.zoom = newZoomSpeed;
        controls.zoomSpeed = controls.baseSettings.zoomSpeed * newZoomSpeed;
      }

      //Radians
      void setFovLimit(float newFovLimit) {
        controls.fovLimit = newFovLimit;
      }

      float getMovementSpeed() {
        return controls.multipliers.movement;
      }

      float getMouseSpeed() {
        return controls.multipliers.mouse;
      }

      float getZoomSpeed() {
        return controls.multipliers.zoom;
      }

      //Radians
      float getFovLimit() {
        return controls.fovLimit;
      }
    }

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
