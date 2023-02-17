#ifndef INTERFACE
#define INTERFACE

#include <glm/glm.hpp>

namespace ammonite {
  namespace interface {
    int createLoadingScreen();
    void deleteLoadingScreen(int targetScreenId);
    void setActiveLoadingScreen(int targetScreenId);

    void setLoadingScreenProgress(int targetScreenId, float progress);
    void setLoadingScreenGeometry(int targetScreenId, float width, float height, float heightOffset);
    void setLoadingScreenColours(int targetScreenId, glm::vec3 backgroundColour, glm::vec3 trackColour, glm::vec3 progressColour);
  }
}

#endif
