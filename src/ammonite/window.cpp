#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>

#include <GLFW/glfw3.h>
#include <stb/stb_image.h>

#include "core/windowManager.hpp"
#include "graphics/internal/internalShaders.hpp"

#include "utils/debug.hpp"
#include "utils/logging.hpp"
#include "constants.hpp"
#include "settings.hpp"

#define DEFAULT_TITLE "Ammonite Window"

namespace ammonite {
  namespace window {
    namespace {
      GLFWwindow* windowPtr;
      bool isFullscreen = false;
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
      ammonite::shaders::internal::updateGLCacheSupport();

      //Setup input for window
      internal::setupGlfwInput();

      return 0;
    }

    int createWindow(int width, int height) {
      return createWindow(width, height, DEFAULT_TITLE);
    }

    void destroyWindow() {
      internal::destroyGlfw();
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
    }

    void setFullscreen(bool fullscreen) {
      if (fullscreen == isFullscreen) {
        return;
      }

      //Set window to windowed mode
      if (!fullscreen) {
        int width = ammonite::settings::runtime::getWidth();
        int height = ammonite::settings::runtime::getHeight();
        glfwSetWindowMonitor(windowPtr, NULL, 0, 0, width, height, GLFW_DONT_CARE);
      }

      //Fullscreen on current monitor
      setFullscreenMonitor(getCurrentMonitor());
    }
  }
}
