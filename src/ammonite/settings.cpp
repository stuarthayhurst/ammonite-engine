#include <GLFW/glfw3.h>

#ifdef DEBUG
  #include <iostream>
#endif

namespace ammonite {
  namespace settings {
    namespace controls {
      namespace {
        //Structure to store base speeds, multipliers and calculated speeds
        struct ControlSettings {
          struct BaseControlSettings {
            const float movementSpeed = 3.0f;
            const float mouseSpeed = 0.005f;
            const float zoomSpeed = 1.0f;
          } baseSettings;

          struct ControlMultipliers {
            float movement = 1.0f;
            float mouse = 1.0f;
            float zoom = 1.0f;
          } multipliers;

          //Final sensitivites, exposed through "internal/runtimeSettings.hpp"
          float movementSpeed = baseSettings.movementSpeed;
          float mouseSpeed = baseSettings.mouseSpeed;
          float zoomSpeed = baseSettings.zoomSpeed;
        } controls;
      }

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

      float getMovementSpeed() {
        return controls.multipliers.movement;
      }

      float getMouseSpeed() {
        return controls.multipliers.mouse;
      }

      float getZoomSpeed() {
        return controls.multipliers.zoom;
      }
    }

    namespace graphics {
      namespace {
        struct GraphicsSettings {
          bool vsyncEnabled = true;
          float frameLimit = 0.0f;
          int shadowRes = 1024;
        } graphics;
      }

      namespace internal {
        float* getFrameLimitPtr() {
          return &graphics.frameLimit;
        }

        int* getShadowResPtr() {
          return &graphics.shadowRes;
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
        graphics.frameLimit = frameLimit;
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
    }

    namespace runtime {
      namespace {
        int width = 0, height = 0;
        float aspectRatio = 0.0f;
      }

      namespace internal {
        float* getAspectRatioPtr() {
          return &aspectRatio;
        }

        int* getWidthPtr() {
          return &width;
        }

        int* getHeightPtr() {
          return &height;
        }

        void setWidth(int newWidth) {
          width = newWidth;
          aspectRatio = float(width) / float(height);
        }

        void setHeight(int newHeight) {
          height = newHeight;
          aspectRatio = float(width) / float(height);
        }
      }

      int getWidth() {
        return width;
      }

      int getHeight() {
        return height;
      }
    }
  }
}
