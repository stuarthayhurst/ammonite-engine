#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <thread>
#include <utility>

extern "C" {
  #include <epoxy/gl.h>
  #include <GLFW/glfw3.h>
}

#include "window.hpp"

#include "../camera.hpp"
#include "../enums.hpp"
#include "../input/input.hpp"
#include "../utils/debug.hpp"
#include "../utils/logging.hpp"
#include "../utils/timer.hpp"

//GLFW-specific implementations to support window.hpp

namespace ammonite {
  namespace window {
    namespace internal {
      namespace {
        struct WindowGeom {
          unsigned int width = 0;
          unsigned int height = 0;
          unsigned int xPos = 0;
          unsigned int yPos = 0;
          unsigned int bufferWidth = 0;
          unsigned int bufferHeight = 0;
        };

        /*
         - Store the current geometry
           - Size is for the content
           - Position is for the whole window
        */
        WindowGeom activeWindowGeom;

        /*
         - Store the window geometry to restore after a fullscreen
           - Size is for the whole window
           - Position is for the whole window
        */
        WindowGeom windowGeomRestore;
        bool isWindowFullscreen = false;

        //Initial allowable frame time error
        double maxError = 1.0 / 50000.0;
        ammonite::utils::Timer targetFrameTimer;
      }

      namespace {
        /*
         - Fill storage with height, width and position
         - Conditionally account for decoration
         - isWindowFullscreen must be set correctly
        */
        void storeWindowGeometry(GLFWwindow* windowPtr, WindowGeom* storage,
                                 bool useDecoratedSize, bool useDecoratedPos) {
          int bufferWidth = 0, bufferHeight = 0;
          glfwGetFramebufferSize(windowPtr, &bufferWidth, &bufferHeight);
          storage->bufferWidth = bufferWidth;
          storage->bufferHeight = bufferHeight;

          /*
           - If the window is fullscreen, set the width and height then bail out
           - Ignores useDecoratedSize and useDecoratedPos
          */
          if (isWindowFullscreen) {
            GLFWmonitor* monitorPtr = glfwGetWindowMonitor(windowPtr);
            if (monitorPtr == nullptr) {
              ammonite::utils::warning << "Couldn't determine monitor, window geometry may be incorrect" << std::endl;
            }

            const GLFWvidmode* mode = glfwGetVideoMode(monitorPtr);
            storage->width = mode->width;
            storage->height = mode->height;
            storage->xPos = 0;
            storage->yPos = 0;
            return;
          }

          //Get window frame size, content size and position
          int frameLeft = 0, frameRight = 0, frameTop = 0, frameBottom = 0;
          glfwGetWindowFrameSize(windowPtr, &frameLeft, &frameTop,
                                 &frameRight, &frameBottom);
          int width = 0, height = 0, xPos = 0, yPos = 0;
          glfwGetWindowSize(windowPtr, &width, &height);
          glfwGetWindowPos(windowPtr, &xPos, &yPos);
          storage->width = width;
          storage->height = height;
          storage->xPos = xPos;
          storage->yPos = yPos;

          //Apply frame dimension corrections
          if (useDecoratedSize) {
            storage->width += frameLeft + frameRight;
            storage->height += frameTop + frameBottom;
          }

          if (useDecoratedPos) {
            storage->xPos -= frameLeft;
            storage->yPos -= frameTop;
          }
        }

        GLFWmonitor* getClosestMonitor(GLFWwindow* windowPtr) {
          int monitorCount = 0;
          GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);

          //Get window position and size
          int wx = 0, wy = 0, ww = 0, wh = 0;
          glfwGetWindowPos(windowPtr, &wx, &wy);
          glfwGetWindowSize(windowPtr, &ww, &wh);

          //Find which monitor the window overlaps most with
          int bestOverlap = 0;
          GLFWmonitor* bestMonitor = nullptr;
          for (int i = 0; i < monitorCount; i++) {
            int mx = 0, my = 0, mw = 0, mh = 0;
            const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
            glfwGetMonitorPos(monitors[i], &mx, &my);
            mw = mode->width;
            mh = mode->height;

            const int overlap =
              std::max(0, std::min(wx + ww, mx + mw) - std::max(wx, mx)) *
              std::max(0, std::min(wy + wh, my + mh) - std::max(wy, my));

            if (bestOverlap < overlap) {
              bestOverlap = overlap;
              bestMonitor = monitors[i];
            }
          }

          return bestMonitor;
        }

        unsigned int getMonitorIndex(GLFWmonitor* monitorPtr) {
          int glfwMonitorCount = 0;
          GLFWmonitor** monitorPtrs = glfwGetMonitors(&glfwMonitorCount);

          //Search provided monitor pointers for the target monitor
          for (int i = 0; i < glfwMonitorCount; i++) {
            if (monitorPtrs[i] == monitorPtr) {
              return (unsigned int)i;
            }
          }

          ammonite::utils::warning << "Failed to find monitor for pointer '" \
                                   << monitorPtr << "'" << std::endl;
          return 0;
        }

        GLFWmonitor* getMonitorPtr(unsigned int monitorIndex) {
          int glfwMonitorCount = 0;
          GLFWmonitor** monitorPtrs = glfwGetMonitors(&glfwMonitorCount);

          if (std::cmp_greater_equal(monitorIndex, glfwMonitorCount)) {
            ammonite::utils::warning << "Unknown monitor index '" << monitorIndex << \
              "', only found " << glfwMonitorCount << " monitors" << std::endl;
            monitorIndex = (unsigned int)(glfwMonitorCount - 1);
          }

          return monitorPtrs[monitorIndex];
        }

        //Callback to update height and width on window resize
        void framebufferSizeCallback(GLFWwindow* windowPtr, int, int) {
          storeWindowGeometry(windowPtr, &activeWindowGeom, false, true);
          ammonite::camera::internal::updateMatrices();
        }

        void windowFocusCallback(GLFWwindow*, int focused) {
          //Unbind / bind input with window focus (fixes missing mouse)
          if (focused == 0) {
            ammonite::input::setInputFocus(focused != 0);
          }
        }
      }

      bool setupGlfw(AmmoniteContextEnum contextType) {
        if (glfwInit() == 0) {
          return false;
        }

        //Request OpenGL 4.5, we check everything we need above 3.2 anyway
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);

        //Disable compatibility profile
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        //Disable deprecated features
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

        //Set fullscreen input focus behaviour
        glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);

        //Set requested context type
        if (contextType == AMMONITE_NO_ERROR_CONTEXT) {
          ammoniteInternalDebug << "Creating window with AMMONITE_NO_ERROR_CONTEXT" << std::endl;
          glfwWindowHint(GLFW_CONTEXT_NO_ERROR, GLFW_TRUE);
        } else if (contextType == AMMONITE_DEBUG_CONTEXT) {
          ammoniteInternalDebug << "Creating window with AMMONITE_DEBUG_CONTEXT" << std::endl;
          glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
        }

        return true;
      }

      void destroyGlfw() {
        glfwTerminate();
      }

      void setFocusCallback(GLFWwindow* windowPtr) {
        //Set callback to update input state on window focus
        glfwSetWindowFocusCallback(windowPtr, windowFocusCallback);
      }

      GLFWwindow* createWindow(unsigned int width, unsigned int height) {
        GLFWwindow* windowPtr = glfwCreateWindow((int)width, (int)height, "",
                                                 nullptr, nullptr);
        if (windowPtr == nullptr) {
          glfwTerminate();
          return nullptr;
        }

        //Store initial geometry
        isWindowFullscreen = false;
        storeWindowGeometry(windowPtr, &activeWindowGeom, false, true);

        //Update stored geometry and matrices when resized
        glfwSetFramebufferSizeCallback(windowPtr, framebufferSizeCallback);
        glfwMakeContextCurrent(windowPtr);

        //Set input modes
        glfwSetInputMode(windowPtr, GLFW_STICKY_KEYS, GL_TRUE);
        glfwSetInputMode(windowPtr, GLFW_STICKY_MOUSE_BUTTONS, GL_TRUE);

        //Enable raw mouse motion if supported
        if (glfwRawMouseMotionSupported() != 0) {
          glfwSetInputMode(windowPtr, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        }

        //Initial input poll
        glfwPollEvents();

        return windowPtr;
      }

      void setWindowResizable(GLFWwindow* windowPtr, bool resizable) {
        glfwSetWindowAttrib(windowPtr, GLFW_RESIZABLE, (int)resizable);
      }

      bool getWindowResizable(GLFWwindow* windowPtr) {
        return glfwGetWindowAttrib(windowPtr, GLFW_RESIZABLE) != 0;
      }

      void setTitle(GLFWwindow* windowPtr, const std::string& title) {
        glfwSetWindowTitle(windowPtr, title.c_str());
      }

      void setIcons(GLFWwindow* windowPtr, ImageData* iconData, unsigned int iconCount) {
        GLFWimage* images = new GLFWimage[iconCount];
        for (unsigned int i = 0; i < iconCount; i++) {
          images[i].pixels = iconData[i].data;
          images[i].width = (int)iconData[i].width;
          images[i].height = (int)iconData[i].height;
        }

        glfwSetWindowIcon(windowPtr, (int)iconCount, images);
        delete [] images;
      }

      void setWindowGeometry(GLFWwindow* windowPtr, unsigned int width, unsigned int height,
                             unsigned int xPos, unsigned int yPos, bool useDecoratedSize,
                             bool useDecoratedPos) {
        //Get window frame size
        int frameLeft = 0, frameRight = 0, frameTop = 0, frameBottom = 0;
        glfwGetWindowFrameSize(windowPtr, &frameLeft, &frameTop,
                               &frameRight, &frameBottom);

        //Account for frame size in position, if required
        if (useDecoratedPos) {
          xPos += frameLeft;
          yPos += frameTop;
        }

        //Account for frame size in size, if required
        if (useDecoratedSize) {
          width -= frameLeft + frameRight;
          height -= frameTop + frameBottom;
        }

        //Update the geometry of the window
        glfwSetWindowPos(windowPtr, (int)xPos, (int)yPos);
        glfwSetWindowSize(windowPtr, (int)width, (int)height);
        storeWindowGeometry(windowPtr, &activeWindowGeom, false, true);
      }

      void getWindowGeometry(GLFWwindow* windowPtr, unsigned int* width, unsigned int* height,
                             unsigned int* xPos, unsigned int* yPos, bool useDecoratedSize,
                             bool useDecoratedPos) {
        WindowGeom tempStorage;
        storeWindowGeometry(windowPtr, &tempStorage, useDecoratedSize, useDecoratedPos);

        *width = tempStorage.width;
        *height = tempStorage.height;
        *xPos = tempStorage.xPos;
        *yPos = tempStorage.yPos;
      }

      void setFullscreenMonitor(GLFWwindow* windowPtr, unsigned int monitorIndex) {
        //Convert index to pointer
        GLFWmonitor* monitorPtr = getMonitorPtr(monitorIndex);

        //Set fullscreen mode
        const GLFWvidmode* mode = glfwGetVideoMode(monitorPtr);
        isWindowFullscreen = true;
        glfwSetWindowMonitor(windowPtr, monitorPtr, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);

        //Update active window geometry store
        storeWindowGeometry(windowPtr, &activeWindowGeom, false, true);
      }

      void setFullscreen(GLFWwindow* windowPtr, bool shouldFullscreen, unsigned int monitorIndex) {
        //Handle new window mode
        if (shouldFullscreen) {
          //Store windowed geometry and then fullscreen
          storeWindowGeometry(windowPtr, &windowGeomRestore, true, true);
          setFullscreenMonitor(windowPtr, monitorIndex);
        } else {
          //Work around maximised windows being made fullscreen again
          if (windowGeomRestore.xPos == 0 && windowGeomRestore.yPos == 0) {
            if (windowGeomRestore.width == activeWindowGeom.width &&
                windowGeomRestore.height == activeWindowGeom.height) {
              windowGeomRestore.xPos = 1;
              windowGeomRestore.yPos = 1;
            }
          }

          //Set window to windowed mode, using last geometry
          isWindowFullscreen = false;
          glfwSetWindowMonitor(windowPtr, nullptr,
                               (int)windowGeomRestore.xPos, (int)windowGeomRestore.yPos,
                               (int)windowGeomRestore.width, (int)windowGeomRestore.height,
                               GLFW_DONT_CARE);

          //Update active window geometry
          storeWindowGeometry(windowPtr, &activeWindowGeom, false, true);
        }
      }

      unsigned int getCurrentMonitorIndex(GLFWwindow* windowPtr) {
        GLFWmonitor* monitorPtr = nullptr;
        if (isWindowFullscreen) {
          monitorPtr = glfwGetWindowMonitor(windowPtr);
        } else {
          monitorPtr = getClosestMonitor(windowPtr);
        }

        return getMonitorIndex(monitorPtr);
      }

      unsigned int getMonitorCount() {
        int glfwMonitorCount = 0;
        glfwGetMonitors(&glfwMonitorCount);

        return (unsigned int)glfwMonitorCount;
      }

      bool getFullscreen() {
        return isWindowFullscreen;
      }

      bool shouldWindowClose(GLFWwindow* windowPtr) {
        return glfwWindowShouldClose(windowPtr) != 0;
      }

      float getGraphicsAspectRatio() {
        return (float)activeWindowGeom.bufferWidth / (float)activeWindowGeom.bufferHeight;
      }

      unsigned int getGraphicsWidth() {
        return activeWindowGeom.bufferWidth;
      }

      unsigned int getGraphicsHeight() {
        return activeWindowGeom.bufferHeight;
      }

      /*
       - Display the rendered frame
       - Waits for vertical sync and / or a frame limiter, if set
      */
      void showFrame(GLFWwindow* windowPtr, bool vsync, float frameLimit) {
        //Set correct vertical sync state
        static bool lastVsync = !vsync;
        if (vsync != lastVsync) {
          glfwSwapInterval(int(vsync));
          lastVsync = vsync;
        }

        glfwSwapBuffers(windowPtr);

        //Wait until next frame should be prepared
        const double targetFrameTime = 1.0 / frameLimit;
        if (frameLimit > 1.0f) {
          //Sleep for progressively shorter intervals, until the budget is gone
          while (targetFrameTime - targetFrameTimer.getTime() > maxError) {
            const double spareTime = targetFrameTime - targetFrameTimer.getTime();
            const auto sleepLength = std::chrono::nanoseconds(
                                     int(std::floor(spareTime * 0.05 * 1000000000.0)));
            std::this_thread::sleep_for(sleepLength);
          }

          //Adjust maxError to provide a closer framerate limit
          const double errorAdjustCoeff = 1.01;
          const double currTime = targetFrameTimer.getTime();
          if (currTime < targetFrameTime) {
            maxError /= errorAdjustCoeff;
          } else if (currTime > targetFrameTime) {
            maxError *= errorAdjustCoeff;
          }
        }

        //Start counting for the next frame
        targetFrameTimer.reset();
      }
    }
  }
}
