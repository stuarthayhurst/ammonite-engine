#ifndef ENVIRONMENT
#define ENVIRONMENT

#include <vector>
#include <string>

namespace ammonite {
  namespace environment {
    namespace skybox {
      int getActiveSkybox();
      void setActiveSkybox(int skyboxId);

      int createSkybox(std::vector<std::string> texturePaths, bool* externalSuccess);
      void deleteSkybox(int skyboxId);
    }
  }
}

#endif
