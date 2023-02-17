#include <map>
#include <iostream>

#include "internal/interfaceTracker.hpp"

#include "utils/logging.hpp"
#include "internal/internalDebug.hpp"

namespace ammonite {
  namespace interface {
    namespace {
      int activeLoadingScreenId = 0;
      int totalLoadingScreens = 0;
      std::map<int, LoadingScreen> loadingScreenTracker;
    }

    //Internally exposed functions
    namespace internal {
      std::map<int, LoadingScreen>* getLoadingScreenTracker() {
        return &loadingScreenTracker;
      }

      int getActiveLoadingScreenId() {
        return activeLoadingScreenId;
      }
    }

    int createLoadingScreen() {
      totalLoadingScreens++;

      //Create and track the loading screen
      LoadingScreen loadingScreen;
      int screenId = totalLoadingScreens;
      loadingScreenTracker[screenId] = loadingScreen;

      return screenId;
    }

    void deleteLoadingScreen(int targetScreenId) {
      //Set as inactive if the target is active, then delete
      if (loadingScreenTracker.contains(targetScreenId)) {
        if (activeLoadingScreenId == targetScreenId) {
          activeLoadingScreenId = 0;
        }

        loadingScreenTracker.erase(targetScreenId);
      } else {
        std::cerr << ammonite::utils::warning << "Loading screen " << targetScreenId << " doesn't exist" << std::endl;
      }
    }

    void setActiveLoadingScreen(int targetScreenId) {
      //Change the active loading screen, if it exists
      if (loadingScreenTracker.contains(targetScreenId)) {
        activeLoadingScreenId = targetScreenId;
      } else if (targetScreenId == 0) {
        activeLoadingScreenId = 0;
      } else {
        std::cerr << ammonite::utils::warning << "Loading screen " << targetScreenId << " doesn't exist" << std::endl;
      }
    }

    void setLoadingScreenProgress(int targetScreenId, float progress) {
      if (loadingScreenTracker.contains(targetScreenId)) {
        loadingScreenTracker[targetScreenId].progress = progress;
      } else {
        std::cerr << ammonite::utils::warning << "Loading screen " << targetScreenId << " doesn't exist" << std::endl;
      }
    }

    void setLoadingScreenGeometry(int targetScreenId, float width, float height, float heightOffset) {
      if (loadingScreenTracker.contains(targetScreenId)) {
        loadingScreenTracker[targetScreenId].width = width;
        loadingScreenTracker[targetScreenId].height = height;
        loadingScreenTracker[targetScreenId].heightOffset = heightOffset;
      } else {
        std::cerr << ammonite::utils::warning << "Loading screen " << targetScreenId << " doesn't exist" << std::endl;
      }
    }

    void setLoadingScreenColours(int targetScreenId, glm::vec3 backgroundColour, glm::vec3 progressColour) {
      if (loadingScreenTracker.contains(targetScreenId)) {
        loadingScreenTracker[targetScreenId].backgroundColour = backgroundColour;
        loadingScreenTracker[targetScreenId].progressColour = progressColour;
      } else {
        std::cerr << ammonite::utils::warning << "Loading screen " << targetScreenId << " doesn't exist" << std::endl;
      }
    }

  }
}
