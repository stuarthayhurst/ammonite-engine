#include <iostream>
#include <unordered_map>

#include "splash.hpp"

#include "utils/id.hpp"
#include "utils/logging.hpp"

namespace ammonite {
  namespace splash {
    namespace {
      AmmoniteId activeSplashScreenId = 0;
      AmmoniteId lastSplashScreenId = 0;
      std::unordered_map<AmmoniteId, internal::SplashScreen> splashScreenTracker;
    }

    //Internally exposed functions
    namespace internal {
      //Pointer is only valid until splashScreenTracker is modified
      SplashScreen* getSplashScreenPtr(AmmoniteId splashScreenId) {
        return &splashScreenTracker[splashScreenId];
      }
    }

    AmmoniteId createSplashScreen() {
      //Create and track the splash screen
      const internal::SplashScreen splashScreen;
      const AmmoniteId screenId = utils::internal::setNextId(&lastSplashScreenId,
                                                             splashScreenTracker);
      splashScreenTracker[screenId] = splashScreen;

      return screenId;
    }

    void deleteSplashScreen(AmmoniteId targetScreenId) {
      //Set as inactive if the target is active, then delete
      if (splashScreenTracker.contains(targetScreenId)) {
        if (activeSplashScreenId == targetScreenId) {
          activeSplashScreenId = 0;
        }

        splashScreenTracker.erase(targetScreenId);
      } else {
        ammonite::utils::warning << "Requested splash screen doesn't exist (ID " \
                                 << targetScreenId << ")" << std::endl;
      }
    }

    void setActiveSplashScreen(AmmoniteId targetScreenId) {
      //Change the active splash screen, if it exists
      if (splashScreenTracker.contains(targetScreenId)) {
        activeSplashScreenId = targetScreenId;
      } else if (targetScreenId == 0) {
        activeSplashScreenId = 0;
      } else {
        ammonite::utils::warning << "Requested splash screen doesn't exist (ID " \
                                 << targetScreenId << ")" << std::endl;
      }
    }

    AmmoniteId getActiveSplashScreenId() {
      return activeSplashScreenId;
    }

    void setSplashScreenProgress(AmmoniteId targetScreenId, float progress) {
      if (splashScreenTracker.contains(targetScreenId)) {
        splashScreenTracker[targetScreenId].progress = progress;
      } else {
        ammonite::utils::warning << "Requested splash screen doesn't exist (ID " \
                                 << targetScreenId << ")" << std::endl;
      }
    }

    void setSplashScreenGeometry(AmmoniteId targetScreenId, float width, float height,
                                  float heightOffset) {
      if (splashScreenTracker.contains(targetScreenId)) {
        splashScreenTracker[targetScreenId].width = width;
        splashScreenTracker[targetScreenId].height = height;
        splashScreenTracker[targetScreenId].heightOffset = heightOffset;
      } else {
           ammonite::utils::warning << "Requested splash screen doesn't exist (ID " \
                                 << targetScreenId << ")" << std::endl;
      }
    }

    void setSplashScreenColours(AmmoniteId targetScreenId, glm::vec3 backgroundColour,
                                 glm::vec3 trackColour, glm::vec3 progressColour) {
      if (splashScreenTracker.contains(targetScreenId)) {
        splashScreenTracker[targetScreenId].backgroundColour = backgroundColour;
        splashScreenTracker[targetScreenId].trackColour = trackColour;
        splashScreenTracker[targetScreenId].progressColour = progressColour;
      } else {
           ammonite::utils::warning << "Requested splash screen doesn't exist (ID " \
                                 << targetScreenId << ")" << std::endl;
      }
    }

  }
}
