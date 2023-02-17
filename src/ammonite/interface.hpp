#ifndef INTERFACE
#define INTERFACE

namespace ammonite {
  namespace interface {
    int createLoadingScreen();
    void deleteLoadingScreen(int targetScreenId);
    void setActiveLoadingScreen(int targetScreenId);
    void updateLoadingScreen(int targetScreenId, float progress);
  }
}

#endif
