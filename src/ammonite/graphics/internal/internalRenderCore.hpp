#ifndef INTERNALRENDERERCORE
#define INTERNALRENDERERCORE

/* Internally exposed header:
 - Expose rendering core to render interface
 - Allow access to internally set settings
 - Allow access to pointers storing settings values
*/

namespace ammonite {
  namespace renderer {
    namespace setup {
      namespace internal {
        bool checkGPUCapabilities(int* failureCount);
        bool createShaders(const char* shaderPath, bool* externalSuccess);
        void setupOpenGLObjects();
        void deleteShaders();
        void destroyOpenGLObjects();
        void deleteModelCache();
      }
    }

    namespace internal {
      void internalDrawFrame();
      void internalDrawLoadingScreen(int loadingScreenId);
    }

    namespace settings {
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
