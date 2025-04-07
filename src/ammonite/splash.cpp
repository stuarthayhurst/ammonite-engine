#include <iostream>
#include <map>

#include "splash.hpp"

#include "types.hpp"
#include "utils/id.hpp"
#include "utils/logging.hpp"

namespace ammonite {
  namespace splash {
    namespace {
      AmmoniteId activeLoadingScreenId = 0;
      AmmoniteId lastLoadingScreenId = 0;
      std::map<AmmoniteId, internal::LoadingScreen> loadingScreenTracker;
    }

    //Internally exposed functions
    namespace internal {
      LoadingScreen* getLoadingScreenPtr(AmmoniteId loadingScreenId) {
        return &loadingScreenTracker[loadingScreenId];
      }
    }

    AmmoniteId createLoadingScreen() {
      //Create and track the loading screen
      const internal::LoadingScreen loadingScreen;
      const AmmoniteId screenId = utils::internal::setNextId(&lastLoadingScreenId,
                                                             loadingScreenTracker);
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
        ammonite::utils::warning << "Requested loading screen doesn't exist (ID " \
                                 << targetScreenId << ")" << std::endl;
      }
    }

    void setActiveLoadingScreen(AmmoniteId targetScreenId) {
      //Change the active loading screen, if it exists
      if (loadingScreenTracker.contains(targetScreenId)) {
        activeLoadingScreenId = targetScreenId;
      } else if (targetScreenId == 0) {
        activeLoadingScreenId = 0;
      } else {
        ammonite::utils::warning << "Requested loading screen doesn't exist (ID " \
                                 << targetScreenId << ")" << std::endl;
      }
    }

    AmmoniteId getActiveLoadingScreenId() {
      return activeLoadingScreenId;
    }

    void setLoadingScreenProgress(AmmoniteId targetScreenId, float progress) {
      if (loadingScreenTracker.contains(targetScreenId)) {
        loadingScreenTracker[targetScreenId].progress = progress;
      } else {
        ammonite::utils::warning << "Requested loading screen doesn't exist (ID " \
                                 << targetScreenId << ")" << std::endl;
      }
    }

    void setLoadingScreenGeometry(AmmoniteId targetScreenId, float width, float height,
                                  float heightOffset) {
      if (loadingScreenTracker.contains(targetScreenId)) {
        loadingScreenTracker[targetScreenId].width = width;
        loadingScreenTracker[targetScreenId].height = height;
        loadingScreenTracker[targetScreenId].heightOffset = heightOffset;
      } else {
           ammonite::utils::warning << "Requested loading screen doesn't exist (ID " \
                                 << targetScreenId << ")" << std::endl;
      }
    }

    void setLoadingScreenColours(AmmoniteId targetScreenId, glm::vec3 backgroundColour,
                                 glm::vec3 trackColour, glm::vec3 progressColour) {
      if (loadingScreenTracker.contains(targetScreenId)) {
        loadingScreenTracker[targetScreenId].backgroundColour = backgroundColour;
        loadingScreenTracker[targetScreenId].trackColour = trackColour;
        loadingScreenTracker[targetScreenId].progressColour = progressColour;
      } else {
           ammonite::utils::warning << "Requested loading screen doesn't exist (ID " \
                                 << targetScreenId << ")" << std::endl;
      }
    }

  }
}
