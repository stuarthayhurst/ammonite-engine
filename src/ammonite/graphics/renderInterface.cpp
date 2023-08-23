#include <iostream>

#include <GLFW/glfw3.h>

#include "../utils/timer.hpp"
#include "../utils/logging.hpp"
#include "../windowManager.hpp"

#include "internal/internalRenderCore.hpp"
#include "../internal/interfaceTracker.hpp"

/*
 - Externally exposed renderer functions
*/

namespace ammonite {
  namespace renderer {
    namespace {
      long long int totalFrames = 0;
      double frameTime = 0.0;
    }

    namespace setup {
      void setupRenderer(const char* shaderPath, bool* externalSuccess) {
        //Start a timer to measure load time
        ammonite::utils::Timer loadTimer;

        GLFWwindow* window = ammonite::window::getWindowPtr();

        //Check GPU supported required extensions
        int failureCount = 0;
        if (!internal::checkGPUCapabilities(&failureCount)) {
          ammonite::utils::error << failureCount << " required extension(s) unsupported" << std::endl;
          *externalSuccess = false;
          return;
        }

        internal::connectWindow(window);
        if (!internal::createShaders(shaderPath, externalSuccess)) {
          return;
        }
        internal::setupOpenGLObjects();

        //Output time taken to load renderer
        ammonite::utils::status << "Loaded renderer in " << loadTimer.getTime() << "s" << std::endl;
      }
    }

    long long int getTotalFrames() {
     return totalFrames;
    }

    double getFrameTime() {
      return frameTime;
    }

    void drawFrame() {
      //Increase frame counters
      static int frameCount = 0;
      int loadingScreenId = ammonite::interface::internal::getActiveLoadingScreenId();
      if (loadingScreenId == -1) {
        totalFrames++;
        frameCount++;
      }

      //Every tenth of a second, update the frame time
      static ammonite::utils::Timer frameTimer;
      double deltaTime = frameTimer.getTime();
      if (deltaTime >= 0.1f) {
        frameTime = deltaTime / frameCount;
        frameTimer.reset();
        frameCount = 0;
      }

      //Offload rest of frame drawing to helpers
      if (loadingScreenId == -1) {
        internal::internalDrawFrame();
      } else {
        internal::internalDrawLoadingScreen(loadingScreenId);
      }
    }
  }
}
