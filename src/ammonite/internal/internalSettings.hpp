#ifndef INTERNALSETTINGS
#define INTERNALSETTINGS

/* Internally exposed header:
 - Allow access to internally set settings
 - Allow access to pointers storing settings values
*/

namespace ammonite {
  namespace settings {
    namespace controls {
      namespace internal {
        float* getMovementSpeedPtr();
        float* getMouseSpeedPtr();
        float* getZoomSpeedPtr();
        float* getFovLimitPtr();
      }
    }

    namespace graphics {
      namespace internal {
        float* getFrameLimitPtr();
        int* getShadowResPtr();
        float* getShadowFarPlanePtr();
        bool* getGammaCorrectionPtr();
      }
    }

    namespace runtime {
      namespace internal {
        float* getAspectRatioPtr();
        int* getWidthPtr();
        int* getHeightPtr();

        void setWidth(int width);
        void setHeight(int height);
      }
    }

  }
}

#endif
