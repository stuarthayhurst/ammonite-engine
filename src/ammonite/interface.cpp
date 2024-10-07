#include <map>
#include <iostream>

#include "internal/interfaceTracker.hpp"

#include "utils/logging.hpp"
#include "types.hpp"

namespace ammonite {
  namespace interface {
    namespace {
      AmmoniteId activeLoadingScreenId = 0;
      int totalLoadingScreens = 0;
      std::map<AmmoniteId, LoadingScreen> loadingScreenTracker;
    }

    //Internally exposed functions
    namespace internal {
      std::map<AmmoniteId, LoadingScreen>* getLoadingScreenTracker() {
        return &loadingScreenTracker;
      }

      AmmoniteId getActiveLoadingScreenId() {
        return activeLoadingScreenId;
      }
    }

    AmmoniteId createLoadingScreen() {
      totalLoadingScreens++;

      //Create and track the loading screen
      LoadingScreen loadingScreen;
      AmmoniteId screenId = totalLoadingScreens;
      loadingScreenTracker[screenId] = loadingScreen;

      return screenId;
    }

    void deleteLoadingScreen(AmmoniteId targetScreenId) {
      //Set as inactive if the target is active, then delete
      if (loadingScreenTracker.contains(targetScreenId)) {
        if (activeLoadingScreenId == targetScreenId) {
          activeLoadingScreenId = 0;
        }

        loadingScreenTracker.erase(targetScreenId);
      } else {
        ammonite::utils::warning << "Loading screen " << targetScreenId << " doesn't exist" << std::endl;
      }
    }

    void setActiveLoadingScreen(AmmoniteId targetScreenId) {
      //Change the active loading screen, if it exists
      if (loadingScreenTracker.contains(targetScreenId)) {
        activeLoadingScreenId = targetScreenId;
      } else if (targetScreenId == 0) {
        activeLoadingScreenId = 0;
      } else {
        ammonite::utils::warning << "Loading screen " << targetScreenId << " doesn't exist" << std::endl;
      }
    }

    AmmoniteId getActiveLoadingScreen() {
      return activeLoadingScreenId;
    }

    void setLoadingScreenProgress(AmmoniteId targetScreenId, float progress) {
      if (loadingScreenTracker.contains(targetScreenId)) {
        loadingScreenTracker[targetScreenId].progress = progress;
      } else {
        ammonite::utils::warning << "Loading screen " << targetScreenId << " doesn't exist" << std::endl;
      }
    }

    void setLoadingScreenGeometry(AmmoniteId targetScreenId, float width, float height,
                                  float heightOffset) {
      if (loadingScreenTracker.contains(targetScreenId)) {
        loadingScreenTracker[targetScreenId].width = width;
        loadingScreenTracker[targetScreenId].height = height;
        loadingScreenTracker[targetScreenId].heightOffset = heightOffset;
      } else {
        ammonite::utils::warning << "Loading screen " << targetScreenId << " doesn't exist" << std::endl;
      }
    }

    void setLoadingScreenColours(AmmoniteId targetScreenId, glm::vec3 backgroundColour,
                                 glm::vec3 trackColour, glm::vec3 progressColour) {
      if (loadingScreenTracker.contains(targetScreenId)) {
        loadingScreenTracker[targetScreenId].backgroundColour = backgroundColour;
        loadingScreenTracker[targetScreenId].trackColour = trackColour;
        loadingScreenTracker[targetScreenId].progressColour = progressColour;
      } else {
        ammonite::utils::warning << "Loading screen " << targetScreenId << " doesn't exist" << std::endl;
      }
    }

  }
}
