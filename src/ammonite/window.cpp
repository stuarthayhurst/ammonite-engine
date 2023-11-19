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
    }

    GLFWwindow* getWindowPtr() {
      return windowPtr;
    }

    void requestContextType(AmmoniteEnum contextType) {
      if (windowPtr != NULL) {
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

    void useIconDir(const char* iconDirPath) {
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

      //Read image data
      GLFWimage images[pngFiles.size()];
      for (unsigned int i = 0; i < pngFiles.size(); i++) {
        images[i].pixels = stbi_load(pngFiles[i].c_str(), &images[i].width, &images[i].height, nullptr, 4);
      }

      //Pass icons to glfw
      if (pngFiles.size() != 0) {
        glfwSetWindowIcon(windowPtr, pngFiles.size(), images);
      }

      //Free the data
      for (unsigned int i = 0; i < pngFiles.size(); i++) {
        stbi_image_free(images[i].pixels);
      }
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

      //Set window to windowed mode
      if (!fullscreen) {
        int width = ammonite::settings::runtime::getWidth();
        int height = ammonite::settings::runtime::getHeight();
        int x = 0, y = 0;

        //Get the current monitor to window to
        GLFWmonitor* currentMonitor = glfwGetWindowMonitor(windowPtr);
        if (currentMonitor != nullptr) {
          glfwGetMonitorPos(currentMonitor, &x, &y);
        }

        glfwSetWindowMonitor(windowPtr, NULL, ++x, ++y, width, height, GLFW_DONT_CARE);

        isFullscreen = false;
        return;
      }

      //Fullscreen on current monitor
      setFullscreenMonitor(getCurrentMonitor());
    }
  }
}
