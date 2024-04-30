#ifndef INTERNALSETTINGS
#define INTERNALSETTINGS

/* Internally exposed header:
 - Allow access to internally set settings
 - Allow access to pointers storing settings values
*/

namespace ammonite {
  namespace settings {
    namespace graphics {
      namespace post {
        namespace internal {
          bool* getFocalDepthEnabledPtr();
          float* getFocalDepthPtr();
          float* getBlurStrengthPtr();
        }
      }
      namespace internal {
        float* getFrameLimitPtr();
        int* getShadowResPtr();
        float* getRenderResMultiplierPtr();
        int* getAntialiasingSamplesPtr();
        float* getRenderFarPlanePtr();
        float* getShadowFarPlanePtr();
        bool* getGammaCorrectionPtr();
      }
    }
  }
}

#endif
