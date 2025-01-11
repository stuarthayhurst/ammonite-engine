#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "internalCamera.hpp"

#include "../enums.hpp"
#include "../utils/logging.hpp"
#include "../utils/debug.hpp"

namespace ammonite {
  namespace window {
    namespace internal {
      namespace {
        GLFWwindow* windowPtr = nullptr;
        AmmoniteEnum requestedContextType = AMMONITE_DEFAULT_CONTEXT;

        struct WindowGeom {
          unsigned int width = 0;
          unsigned int height = 0;
          unsigned int xPos = 0;
          unsigned int yPos = 0;
          float aspectRatio = 0.0f; //Aspect ratio is always for window content
        };

        //Store the current geometry, where size is for the content and position is for the window
        WindowGeom activeWindowGeom;
        //Save geometry to restore from fullscreen, using size and position for the whole window
        WindowGeom windowGeomRestore;
        bool isWindowFullscreen = false;
      }

      namespace {
        static GLFWmonitor* getClosestMonitor() {
          int monitorCount = 0;
          GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

          //Get window position and size
          int wx = 0, wy = 0, ww = 0, wh = 0;
          glfwGetWindowPos(windowPtr, &wx, &wy);
          glfwGetWindowSize(windowPtr, &ww, &wh);

          //Find which monitor the window overlaps most with
          int bestOverlap = 0;
          GLFWmonitor *bestMonitor = nullptr;
          for (int i = 0; i < monitorCount; i++) {
            int mx = 0, my = 0, mw = 0, mh = 0;
            const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
            glfwGetMonitorPos(monitors[i], &mx, &my);
            mw = mode->width;
            mh = mode->height;

            int overlap =
              std::max(0, std::min(wx + ww, mx + mw) - std::max(wx, mx)) *
              std::max(0, std::min(wy + wh, my + mh) - std::max(wy, my));

            if (bestOverlap < overlap) {
              bestOverlap = overlap;
              bestMonitor = monitors[i];
            }
          }

          return bestMonitor;
        }

        //Fill storage with height, width, position and aspect ratio
        // - Conditionally account for decoration
        // - isWindowFullscreen must be set correctly
        static void storeWindowGeometry(WindowGeom* storage, bool useDecoratedSize,
                                        bool useDecoratedPos) {
          //If the window is fullscreen, set the width, height, aspect ratio and bail out
          // - Ignore useDecoratedSize and useDecoratedPos
          if (isWindowFullscreen) {
            GLFWmonitor* monitorPtr = glfwGetWindowMonitor(windowPtr);
            const GLFWvidmode* mode = glfwGetVideoMode(monitorPtr);
            storage->width = mode->width;
            storage->height = mode->height;
            storage->aspectRatio = (float)(storage->width) / (float)(storage->height);
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

          storage->aspectRatio = (float)(storage->width) / (float)(storage->height);

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

        //Callback to update height and width on window resize
        static void windowSizeCallback(GLFWwindow*, int, int) {
          storeWindowGeometry(&activeWindowGeom, false, true);
          ammonite::camera::internal::updateMatrices();
        }
      }

      GLFWwindow* getWindowPtr() {
        return windowPtr;
      }

      bool setupGlfw() {
        if (!glfwInit()) {
          return false;
        }

        //Set minimum version to OpenGL 3.2+
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

        //Disable compatibility profile
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        //Disable deprecated features
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

        //Set fullscreen input focus behaviour
        glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);

        //Set requested context type
        if (requestedContextType == AMMONITE_NO_ERROR_CONTEXT) {
          ammoniteInternalDebug << "Creating window with AMMONITE_NO_ERROR_CONTEXT" << std::endl;
          glfwWindowHint(GLFW_CONTEXT_NO_ERROR, GLFW_TRUE);
        } else if (requestedContextType == AMMONITE_DEBUG_CONTEXT) {
          ammoniteInternalDebug << "Creating window with AMMONITE_DEBUG_CONTEXT" << std::endl;
          glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
        }

        return true;
      }

      bool setupGlew() {
        glewExperimental = GL_TRUE;
        GLenum err = glewInit();
        if (err != GLEW_OK) {
          //Workaround for GLEW issues on Wayland (requires GLFW 3.4+)
#if (GLFW_VERSION_MAJOR > 3) || ((GLFW_VERSION_MAJOR == 3) && (GLFW_VERSION_MINOR >= 4))
          int platform = glfwGetPlatform();
          if (err == GLEW_ERROR_NO_GLX_DISPLAY && platform == GLFW_PLATFORM_WAYLAND) {
            ammonite::utils::warning << "Wayland detected, ignoring GLEW_ERROR_NO_GLX_DISPLAY" \
                                     << std::endl;
            return true;
          }
#else
          #pragma message("GLFW 3.4 unavailable, skipping Wayland checks")
#endif

          ammonite::utils::error << glewGetErrorString(err) << std::endl;
          return false;
        }

        return true;
      }

      //Set input and cursor modes for window
      void setupGlfwInput() {
        glfwSetInputMode(windowPtr, GLFW_STICKY_KEYS, GL_TRUE);
        glfwSetInputMode(windowPtr, GLFW_STICKY_MOUSE_BUTTONS, GL_TRUE);

        //Enable raw mouse motion if supported
        if (glfwRawMouseMotionSupported()) {
          glfwSetInputMode(windowPtr, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        }

        //Initial input poll
        glfwPollEvents();
      }

      void destroyGlfw() {
        glfwTerminate();
      }

      void setContextType(AmmoniteEnum contextType) {
        requestedContextType = contextType;
      }

      GLFWwindow* createWindow(unsigned int width, unsigned int height, std::string title) {
        windowPtr = glfwCreateWindow((int)width, (int)height, title.c_str(), nullptr, nullptr);
        if (windowPtr == nullptr) {
          glfwTerminate();
          return nullptr;
        }

        isWindowFullscreen = false;
        storeWindowGeometry(&activeWindowGeom, false, true);

        //Update stored geometry and matrices when resized
        glfwSetWindowSizeCallback(windowPtr, windowSizeCallback);

        glfwMakeContextCurrent(windowPtr);
        return windowPtr;
      }

      //Set decorated window size and position, for non-fullscreen windows only
      void setWindowGeometry(unsigned int width, unsigned int height, unsigned int xPos,
                             unsigned int yPos, bool useDecoratedPos) {
        //Don't allow setting window geometry for fullscreen windows
        if (isWindowFullscreen) {
          return;
        }

        //Get window frame size and account for it, if required
        if (useDecoratedPos) {
          int frameLeft = 0, frameRight = 0, frameTop = 0, frameBottom = 0;
          glfwGetWindowFrameSize(windowPtr, &frameLeft, &frameTop,
                                 &frameRight, &frameBottom);

          //Apply frame dimension corrections
          width -= frameLeft + frameRight;
          height -= frameTop + frameBottom;
          xPos += frameLeft;
          yPos += frameTop;
        }

        //Update the geometry of the window
        glfwSetWindowPos(windowPtr, (int)xPos, (int)yPos);
        glfwSetWindowSize(windowPtr, (int)width, (int)height);
        storeWindowGeometry(&activeWindowGeom, false, true);
      }

      //Return geometry information for the active window
      void getWindowGeometry(unsigned int* width, unsigned int* height, unsigned int* xPos,
                             unsigned int* yPos, bool useDecoratedPos) {
        WindowGeom tempStorage;
        storeWindowGeometry(&tempStorage, useDecoratedPos, useDecoratedPos);

        *width = tempStorage.width;
        *height = tempStorage.height;
        *xPos = tempStorage.xPos;
        *yPos = tempStorage.yPos;
      }

      void setFullscreenMonitor(GLFWmonitor* monitor) {
        //Set fullscreen mode
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(windowPtr, monitor, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);

        //Update active window geometry store
        isWindowFullscreen = true;
        storeWindowGeometry(&activeWindowGeom, false, true);
      }

      void setFullscreen(bool shouldFullscreen) {
        if (shouldFullscreen == isWindowFullscreen) {
          return;
        }

        //Handle new window mode
        if (shouldFullscreen) {
          //Store windowed geometry and then fullscreen
          storeWindowGeometry(&windowGeomRestore, true, true);
          setFullscreenMonitor(getClosestMonitor());
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
          glfwSetWindowMonitor(windowPtr, nullptr, (int)windowGeomRestore.xPos, (int)windowGeomRestore.yPos,
                               (int)windowGeomRestore.width, (int)windowGeomRestore.height, GLFW_DONT_CARE);

          //Update active window geometry
          isWindowFullscreen = false;
          storeWindowGeometry(&activeWindowGeom, false, true);
        }
      }

      //Works when fullscreen or windowed
      GLFWmonitor* getCurrentMonitor() {
        return isWindowFullscreen ? glfwGetWindowMonitor(windowPtr) : getClosestMonitor();
      }

      bool getFullscreen() {
        return isWindowFullscreen;
      }

      float getAspectRatio() {
        return activeWindowGeom.aspectRatio;
      }

      unsigned int getWidth() {
        return activeWindowGeom.width;
      }

      unsigned int getHeight() {
        return activeWindowGeom.height;
      }
    }
  }
}
