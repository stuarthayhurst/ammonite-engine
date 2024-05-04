#include <thread>
#include <cmath>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "internal/internalRenderCore.hpp"
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

        //Wait until next frame should be prepared
        static ammonite::utils::Timer targetFrameTimer;
        static float* frameLimitPtr = renderer::settings::internal::getFrameLimitPtr();
        if (*frameLimitPtr > 1.0f) {
          //Initial length of allowable error in seconds
          static double maxError = (1.0 / 50000.0);
          #define errorAdjustCoeff 1.01

          //Figure out time budget remaining
          double const targetFrameTime = 1.0 / *frameLimitPtr;
          double spareTime = targetFrameTime - targetFrameTimer.getTime();

          //Sleep for successively shorter intervals until the frametime budget is gone
          while (targetFrameTime - targetFrameTimer.getTime() > maxError) {
            spareTime = targetFrameTime - targetFrameTimer.getTime();
            const auto sleepLength = std::chrono::nanoseconds(
                                     int(std::floor(spareTime * 0.05 * 1000000000.0)));
            std::this_thread::sleep_for(sleepLength);
          }

          //Adjust maxError to provide a closer framerate limit
          const double currTime = targetFrameTimer.getTime();
          if (currTime < targetFrameTime) {
            maxError *= (1.0 / errorAdjustCoeff);
          } else if (currTime > targetFrameTime) {
            maxError *= (errorAdjustCoeff);
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
