#ifndef ENVIRONMENT
#define ENVIRONMENT

namespace ammonite {
  namespace environment {
    namespace skybox {
      int getActiveSkybox();
      void setActiveSkybox(int skyboxId);

      int createSkybox(const char* texturePaths[6], bool* externalSuccess);
      int createSkybox(const char* texturePaths[6], bool flipTextures, bool srgbTextures, bool* externalSuccess);
      void deleteSkybox(int skyboxId);
    }
  }
}

#endif
