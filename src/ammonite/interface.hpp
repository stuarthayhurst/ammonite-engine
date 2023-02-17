#ifndef INTERFACE
#define INTERFACE

namespace ammonite {
  namespace interface {
    int createLoadingScreen();
    void deleteLoadingScreen(int targetScreenId);
    void setActiveLoadingScreen(int targetScreenId);

    void setLoadingScreenProgress(int targetScreenId, float progress);
    void setLoadingScreenGeometry(int targetScreenId, float width, float height, float heightOffset);
  }
}

#endif
