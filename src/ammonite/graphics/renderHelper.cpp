#include <thread>
#include <cmath>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../internal/internalSettings.hpp"
#include "../utils/timer.hpp"

/*
 - Generic helpers for the render core
*/

namespace ammonite {
  namespace renderer {
    namespace internal {
      void prepareScreen(int framebufferId, int width, int height, bool depthTest) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
        glViewport(0, 0, width, height);
        if (depthTest) {
          glEnable(GL_DEPTH_TEST);
        } else {
          glDisable(GL_DEPTH_TEST);
        }
      }

      void finishFrame(GLFWwindow* window) {
        //Swap buffers
        glfwSwapBuffers(window);

        //Figure out time budget remaining
        static ammonite::utils::Timer targetFrameTimer;

        //Wait until next frame should be prepared
        static float* frameLimitPtr = ammonite::settings::graphics::internal::getFrameLimitPtr();
        if (*frameLimitPtr != 0.0f) {
          //Length of microsleep and allowable error
          static const double sleepInterval = 1.0 / 100000;
          static const double maxError = (sleepInterval) * 2.0f;
          static const auto sleepLength = std::chrono::nanoseconds(int(std::floor(sleepInterval * 1000000000.0)));

          double const targetFrameTime = 1.0 / *frameLimitPtr;
          double spareTime = targetFrameTime - targetFrameTimer.getTime();

          //Sleep for short intervals until the frametime budget is gone
          while (spareTime > maxError) {
            std::this_thread::sleep_for(sleepLength);
            spareTime = targetFrameTime - targetFrameTimer.getTime();
          }
        }

        targetFrameTimer.reset();
      }

      void setWireframe(bool enabled) {
        //Avoid unnecessary polygon mode updates
        static bool oldEnabled = !enabled;
        if (oldEnabled == enabled) {
          return;
        }

        //Change the draw mode
        glPolygonMode(GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL);
        oldEnabled = enabled;
      }
    }
  }
}
