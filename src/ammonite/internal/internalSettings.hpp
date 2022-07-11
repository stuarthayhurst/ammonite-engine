#ifndef INTERNALSETTINGS
#define INTERNALSETTINGS

namespace ammonite {
  namespace settings {
    namespace controls {
      namespace internal {
        float* getMovementSpeedPtr();
        float* getMouseSpeedPtr();
        float* getZoomSpeedPtr();
      }
    }

    namespace graphics {
      namespace internal {
        float* getFrameLimitPtr();
        int* getShadowResPtr();
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
