#include <cstdint>
#include <iostream>
#include <string>

#include "renderer.hpp"

#include "../camera.hpp"
#include "../enums.hpp"
#include "../lighting/lighting.hpp"
#include "../splash.hpp"
#include "../utils/id.hpp"
#include "../utils/logging.hpp"
#include "../utils/thread.hpp"
#include "../utils/timer.hpp"
#include "../window/window.hpp"

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
      bool setupRenderer(const std::string& shaderPath) {
        //Start a timer to measure load time
        ammonite::utils::Timer loadTimer;

        //Create a thread pool
        if (!ammonite::utils::thread::createThreadPool(0)) {
          ammonite::utils::error << "Failed to create thread pool" << std::endl;
          return false;
        }

        ammonite::utils::status << "Created thread pool with " \
                                << ammonite::utils::thread::getThreadPoolSize() \
                                << " threads" << std::endl;

        //Check GPU supported required extensions
        unsigned int failureCount = 0;
        if (!internal::checkGPUCapabilities(&failureCount)) {
          ammonite::utils::error << failureCount << " required extension(s) unsupported" << std::endl;
          return false;
        }

        //Create OpenGL objects and shaders
        if (!internal::createShaders(shaderPath)) {
          return false;
        }
        internal::setupOpenGLObjects();

        //Output time taken to load renderer and return
        ammonite::utils::status << "Loaded renderer in " << loadTimer.getTime() << "s" << std::endl;
        return true;
      }

      void destroyRenderer() {
        ammonite::utils::thread::destroyThreadPool();
        internal::deleteShaders();
        internal::destroyOpenGLObjects();
        lighting::internal::destroyLightSystem();
        internal::deleteModelCache();
      }

      void requestContextType(AmmoniteContextEnum contextType) {
        switch (contextType) {
        case AMMONITE_DEFAULT_CONTEXT:
        case AMMONITE_NO_ERROR_CONTEXT:
        case AMMONITE_DEBUG_CONTEXT:
          ammonite::window::internal::setContextType(contextType);
          return;
        default:
          ammonite::utils::warning << "Unknown context type '" << contextType << "' requested" << std::endl;
          return;
        }
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
      const AmmoniteId loadingScreenId = ammonite::splash::getActiveLoadingScreenId();
      if (loadingScreenId == 0) {
        ammonite::lighting::internal::startUpdateLightSources();
        totalFrames++;
        frameCount++;
      }

      //Every tenth of a second, update the frame time
      static ammonite::utils::Timer frameTimer;
      const double deltaTime = frameTimer.getTime();
      if (deltaTime >= 0.1f) {
        frameTime = deltaTime / frameCount;
        frameTimer.reset();
        frameCount = 0;
      }

      //Offload rest of frame drawing to helpers
      if (loadingScreenId == 0) {
        ammonite::camera::internal::updateMatrices();
        internal::internalDrawFrame();
      } else {
        internal::internalDrawLoadingScreen(loadingScreenId);
      }
    }
  }
}
