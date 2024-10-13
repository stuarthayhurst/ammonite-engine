#include <cstdint>
#include <iostream>
#include <string>

#include <GLFW/glfw3.h>

#include "../utils/files.hpp"
#include "../utils/logging.hpp"
#include "../utils/timer.hpp"

#include "../internal/internalInterface.hpp"
#include "../interface.hpp"

#include "internal/internalRenderCore.hpp"
#include "../internal/internalCamera.hpp"
#include "../lighting/internal/internalLighting.hpp"
#include "../utils/internal/threadPool.hpp"

#include "../types.hpp"

/*
 - Externally exposed renderer functions
*/

namespace ammonite {
  namespace renderer {
    namespace {
      uintmax_t totalFrames = 0;
      double frameTime = 0.0;
    }

    namespace setup {
      void setupRenderer(std::string shaderPath, bool* externalSuccess) {
        //Start a timer to measure load time
        ammonite::utils::Timer loadTimer;

        //Create a thread pool
        if (ammonite::utils::thread::internal::createThreadPool(0) == -1) { \
          ammonite::utils::error << "Failed to create thread pool" << std::endl;
          *externalSuccess = false;
          return;
        }

        ammonite::utils::status << "Created thread pool with " \
                                << ammonite::utils::thread::internal::getThreadPoolSize() \
                                << " threads" << std::endl;

        //Check GPU supported required extensions
        unsigned int failureCount = 0;
        if (!internal::checkGPUCapabilities(&failureCount)) {
          ammonite::utils::error << failureCount << " required extension(s) unsupported" << std::endl;
          *externalSuccess = false;
          return;
        }

        if (!internal::createShaders(shaderPath, externalSuccess)) {
          return;
        }
        internal::setupOpenGLObjects();

        //Output time taken to load renderer
        ammonite::utils::status << "Loaded renderer in " << loadTimer.getTime() << "s" << std::endl;
      }

      void destroyRenderer() {
        ammonite::utils::thread::internal::destroyThreadPool();
        internal::deleteShaders();
        internal::destroyOpenGLObjects();
        lighting::internal::destroyLightSystem();
        internal::deleteModelCache();
      }
    }

    uintmax_t getTotalFrames() {
     return totalFrames;
    }

    double getFrameTime() {
      return frameTime;
    }

    void drawFrame() {
      //Increase frame counters
      static int unsigned frameCount = 0;
      AmmoniteId loadingScreenId = ammonite::interface::getActiveLoadingScreenId();
      if (loadingScreenId == 0) {
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
      if (loadingScreenId == 0) {
        ammonite::lighting::internal::updateLightSources();
        ammonite::camera::internal::updateMatrices();
        internal::internalDrawFrame();
      } else {
        internal::internalDrawLoadingScreen(loadingScreenId);
      }
    }
  }
}
