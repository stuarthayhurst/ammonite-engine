#include "../utils/timer.hpp"

#include "internal/internalRenderer.hpp"

namespace ammonite {
  namespace renderer {
    namespace {
      long long int totalFrames = 0;
      double frameTime = 0.0;
    }

    namespace setup {
      //TODO
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
      totalFrames++;
      frameCount++;

      //Every tenth of a second, update the frame time
      static ammonite::utils::Timer frameTimer;
      double deltaTime = frameTimer.getTime();
      if (deltaTime >= 0.1f) {
        frameTime = deltaTime / frameCount;
        frameTimer.reset();
        frameCount = 0;
      }

      //Offload rest of frame drawing to helpers
      internalDrawFrame();
    }
  }
}
