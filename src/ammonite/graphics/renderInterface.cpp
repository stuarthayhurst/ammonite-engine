#include <iostream>

#include <GLFW/glfw3.h>

#include "../utils/debug.hpp"
#include "../utils/timer.hpp"
#include "../utils/logging.hpp"
#include "../core/windowManager.hpp"
#include "../core/threadManager.hpp"

#include "internal/internalRenderCore.hpp"
#include "../internal/interfaceTracker.hpp"
#include "../lighting/internal/internalLighting.hpp"

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

        //Create a thread pool
        if (ammonite::thread::internal::createThreadPool(0) == -1) { \
          ammonite::utils::error << "Failed to create thread pool" << std::endl;
          *externalSuccess = false;
          return;
        }

#ifdef DEBUG
        ammoniteInternalDebug << "Created thread pool with " \
                              << ammonite::thread::internal::getThreadPoolSize() \
                              << " threads" << std::endl;
#endif

        GLFWwindow* window = ammonite::window::internal::getWindowPtr();

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

      void destroyRenderer() {
        ammonite::thread::internal::destroyThreadPool();
        internal::deleteShaders();
        internal::destroyOpenGLObjects();
        internal::deleteModelCache();
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
      static bool* lightDataChanged = ammonite::lighting::internal::getLightSourcesChangedPtr();
      if (loadingScreenId == -1) {
        if (*lightDataChanged) {
          ammonite::lighting::internal::updateLightSources();
        }
        internal::internalDrawFrame();
      } else {
        internal::internalDrawLoadingScreen(loadingScreenId);
      }
    }
  }
}
