#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>

#include <GLFW/glfw3.h>
#include <stb/stb_image.h>

#include "core/windowManager.hpp"
#include "core/inputManager.hpp"
#include "internal/internalInput.hpp"
#include "graphics/internal/internalShaders.hpp"

#include "utils/debug.hpp"
#include "utils/logging.hpp"
#include "settings.hpp"
#include "types.hpp"

#define DEFAULT_TITLE "Ammonite Window"

namespace ammonite {
  namespace window {
    namespace {
      GLFWwindow* windowPtr = nullptr;
      bool isFullscreen = false;
      bool closeWindow = false;

      struct WindowGeom {
        int width = 0;
        int height = 0;
        int xPos = 0;
        int yPos = 0;
      } lastWindowGeom;
    }

    namespace {
      static GLFWmonitor* getCurrentMonitor() {
        int monitorCount;
        GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

        //Get window position and size
        int wx, wy, ww, wh;
        glfwGetWindowPos(windowPtr, &wx, &wy);
        glfwGetWindowSize(windowPtr, &ww, &wh);

        //Find which monitor the window overlaps most with
        int bestOverlap = 0;
        GLFWmonitor *bestMonitor = nullptr;
        for (int i = 0; i < monitorCount; i++) {
          int mx, my, mw, mh;
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

      static void setCloseWindowCallback(std::vector<int>, int, void* userPtr) {
        bool* closeWindowPtr = (bool*)userPtr;
        *closeWindowPtr = true;
      }

      static void storeWindowGeometry(WindowGeom* storage, bool useDecoratedSize,
                                      bool useDecoratedPos) {
        //Get window frame size, content size and position
        int frameLeft = 0, frameRight = 0, frameTop = 0, frameBottom = 0;
        glfwGetWindowFrameSize(windowPtr, &frameLeft, &frameTop,
                               &frameRight, &frameBottom);
        glfwGetWindowSize(windowPtr, &storage->width, &storage->height);
        glfwGetWindowPos(windowPtr, &storage->xPos, &storage->yPos);

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
    }

    GLFWwindow* getWindowPtr() {
      return windowPtr;
    }

    void requestContextType(AmmoniteEnum contextType) {
      if (windowPtr != nullptr) {
        ammonite::utils::warning << "Window already created, ignoring context type request" << std::endl;
        return;
      }

      switch (contextType) {
      case AMMONITE_DEFAULT_CONTEXT:
      case AMMONITE_NO_ERROR_CONTEXT:
      case AMMONITE_DEBUG_CONTEXT:
        internal::setContextType(contextType);
        return;
      default:
        ammonite::utils::warning << "Unknown context type '" << contextType << "' requested" << std::endl;
        return;
      }
    }

    int createWindow(int width, int height, const char* title) {
      //Setup GLFW
      if (internal::setupGlfw() == -1) {
        ammonite::utils::error << "Failed to initialize GLFW" << std::endl;
        return -1;
      }

      //Create window
      windowPtr = internal::createWindow(width, height, title);
      if (windowPtr == nullptr) {
        ammonite::utils::error << "Failed to open window" << std::endl;
        return -1;
      }

      //Setup GLEW
      if (internal::setupGlew() == -1) {
        ammonite::utils::error << "Failed to initialize GLEW" << std::endl;
        return -1;
      }

      //Prompt renderer shader cache support checking
      ammonite::shaders::internal::updateCacheSupport();

      //Setup input for window
      internal::setupGlfwInput();
      ammonite::input::internal::setupInputCallback(windowPtr);
      ammonite::input::internal::setupFocusCallback(windowPtr);

      return 0;
    }

    int createWindow(int width, int height) {
      return createWindow(width, height, DEFAULT_TITLE);
    }

    void destroyWindow() {
      internal::destroyGlfw();
    }

    bool shouldWindowClose() {
      return (closeWindow || glfwWindowShouldClose(windowPtr));
    }

    int registerWindowCloseKeybind(int keycode) {
      return ammonite::input::internal::registerRawKeybind(
                         &keycode, 1, AMMONITE_ALLOW_OVERRIDE, true,
                         setCloseWindowCallback, &closeWindow);
    }

    void setTitle(const char* title) {
      if (title != nullptr) {
        glfwSetWindowTitle(windowPtr, title);
      } else {
        glfwSetWindowTitle(windowPtr, DEFAULT_TITLE);
      }
    }

    void useIcons(const char* iconPaths[], int iconCount) {
      if (iconPaths == nullptr) {
        ammonite::utils::warning << "Failed to load icons (nullptr)" << std::endl;
        return;
      }

      //Read image data
      GLFWimage images[iconCount];
      for (unsigned int i = 0; i < (unsigned)iconCount; i++) {
        if (iconPaths[i] == nullptr) {
          ammonite::utils::warning << "Failed to load icon (nullptr)" << std::endl;
          return;
        }

        images[i].pixels = stbi_load(iconPaths[i], &images[i].width,
                                     &images[i].height, nullptr, 4);

        if (images[i].pixels == nullptr) {
          ammonite::utils::warning << "Failed to load '" << iconPaths[i] << "'" << std::endl;
          return;
        }
      }

      //Pass icons to glfw
      if (iconCount != 0) {
        glfwSetWindowIcon(windowPtr, iconCount, images);
      }

      //Free the data
      for (int i = 0; i < iconCount; i++) {
        stbi_image_free(images[i].pixels);
      }
    }

    void useIcon(const char* iconPath) {
      useIcons(&iconPath, 1);
    }

    void useIconDir(const char* iconDirPath) {
      if (iconDirPath == nullptr) {
        ammonite::utils::warning << "Failed to load icon directory (nullptr)" << std::endl;
        return;
      }

      //Attempt to add all png files to a vector
      std::vector<std::string> pngFiles(0);
      try {
        const std::filesystem::path searchDir{iconDirPath};
        const auto it = std::filesystem::directory_iterator{searchDir};

        for (auto const& fileName : it) {
          std::filesystem::path filePath{fileName};
          if (filePath.extension() == ".png") {
            pngFiles.push_back(std::string(filePath));
          }
        }
      } catch (const std::filesystem::filesystem_error&) {
        ammonite::utils::warning << "Couldn't open '" << iconDirPath << "'" << std::endl;
        return;
      }

      const char* iconPaths[pngFiles.size()];
      for (unsigned int i = 0; i < pngFiles.size(); i++) {
        iconPaths[i] = pngFiles[i].c_str();
      }

      //Hand off to another icon handler
      useIcons(iconPaths, pngFiles.size());
    }

    //Set decorated window size and position
    void setWindowGeometry(int width, int height, int xPos, int yPos) {
      //Don't allow setting window geometry for fullscreen windows
      if (isFullscreen) {
        return;
      }

      //Get window frame size
      int frameLeft = 0, frameRight = 0, frameTop = 0, frameBottom = 0;
      glfwGetWindowFrameSize(windowPtr, &frameLeft, &frameTop,
                             &frameRight, &frameBottom);

      //Apply frame dimension corrections
      width -= frameLeft + frameRight;
      height -= frameTop + frameBottom;
      xPos += frameLeft;
      yPos += frameTop;

      //Update the geometry of the window
      glfwSetWindowPos(windowPtr, xPos, yPos);
      glfwSetWindowSize(windowPtr, width, height);
    }

    //Set pointers to values for a decorated window
    void getWindowGeometry(int* width, int* height, int* xPos, int* yPos) {
      //Don't allow querying window geometry for fullscreen windows
      if (isFullscreen) {
        return;
      }

      WindowGeom tempStorage;
      storeWindowGeometry(&tempStorage, true, true);

      *width = tempStorage.width;
      *height = tempStorage.height;
      *xPos = tempStorage.xPos;
      *yPos = tempStorage.yPos;
    }

    bool getFullscreen() {
      return isFullscreen;
    }

    GLFWmonitor* getFullscreenMonitor() {
      return isFullscreen ? glfwGetWindowMonitor(windowPtr) : nullptr;
    }

    int getMonitors(GLFWmonitor*** monitorsPtr) {
      int monitorCount;
      *monitorsPtr = glfwGetMonitors(&monitorCount);
      return monitorCount;
    }

    void setFullscreenMonitor(GLFWmonitor* monitor) {
      const GLFWvidmode* mode = glfwGetVideoMode(monitor);
      int width = mode->width;
      int height = mode->height;

      //Set fullscreen
      glfwSetWindowMonitor(windowPtr, monitor, 0, 0, width, height, GLFW_DONT_CARE);
      isFullscreen = true;
    }

    void setFullscreen(bool fullscreen) {
      if (fullscreen == isFullscreen) {
        return;
      }

      //Handle new window mode
      if (fullscreen) {
        //Store windowed geometry and then fullscreen
        storeWindowGeometry(&lastWindowGeom, true, true);
        setFullscreenMonitor(getCurrentMonitor());
      } else {
        //Set window to windowed mode, using last geometry
        glfwSetWindowMonitor(windowPtr, nullptr, lastWindowGeom.xPos, lastWindowGeom.yPos,
                             lastWindowGeom.width, lastWindowGeom.height, GLFW_DONT_CARE);

        isFullscreen = false;
      }
    }
  }
}
