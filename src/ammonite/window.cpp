#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <GLFW/glfw3.h>
#include <stb/stb_image.h>

#include "core/windowManager.hpp"
#include "core/inputManager.hpp"
#include "internal/internalInput.hpp"

#include "utils/logging.hpp"
#include "enums.hpp"
#include "settings.hpp"

#define DEFAULT_TITLE "Ammonite Window"

namespace ammonite {
  namespace window {
    namespace {
      GLFWwindow* windowPtr = nullptr;
      bool closeWindow = false;
    }

    namespace {
      static void setCloseWindowCallback(std::vector<int>, int, void* userPtr) {
        *(bool*)userPtr = true;
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
      GLFWimage* images = new GLFWimage[iconCount];
      for (unsigned int i = 0; i < (unsigned)iconCount; i++) {
        if (iconPaths[i] == nullptr) {
          ammonite::utils::warning << "Failed to load icon (nullptr)" << std::endl;
          delete[] images;
          return;
        }

        images[i].pixels = stbi_load(iconPaths[i], &images[i].width,
                                     &images[i].height, nullptr, 4);

        if (images[i].pixels == nullptr) {
          ammonite::utils::warning << "Failed to load '" << iconPaths[i] << "'" << std::endl;
          delete[] images;
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
      delete[] images;
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

      const char** iconPaths = new const char*[pngFiles.size()];
      for (unsigned int i = 0; i < pngFiles.size(); i++) {
        iconPaths[i] = pngFiles[i].c_str();
      }

      //Hand off to another icon handler
      useIcons(iconPaths, pngFiles.size());
      delete [] iconPaths;
    }

    //Hand off to core window manager
    void setWindowGeometry(int width, int height, int xPos, int yPos, bool useDecoratedPos) {
      internal::setWindowGeometry(width, height, xPos, yPos, useDecoratedPos);
    }

    //Hand off to core window manager
    void getWindowGeometry(int* width, int* height, int* xPos, int* yPos, bool useDecoratedPos) {
      int ignore = 0;
      internal::getWindowGeometry((width != nullptr) ? width : &ignore,
                                  (height != nullptr) ? height : &ignore,
                                  (xPos != nullptr) ? xPos : &ignore,
                                  (yPos != nullptr) ? yPos : &ignore,
                                  useDecoratedPos);
    }

    void setWindowResizable(bool resizable) {
      glfwSetWindowAttrib(windowPtr, GLFW_RESIZABLE, resizable);
    }

    bool getWindowResizable() {
      return glfwGetWindowAttrib(windowPtr, GLFW_RESIZABLE);
    }

    bool getFullscreen() {
      return internal::getFullscreen();
    }

    GLFWmonitor* getCurrentMonitor() {
      return internal::getCurrentMonitor();
    }

    int getMonitors(GLFWmonitor*** monitorsPtr) {
      int monitorCount;
      *monitorsPtr = glfwGetMonitors(&monitorCount);
      return monitorCount;
    }

    void setFullscreenMonitor(GLFWmonitor* monitor) {
      if (monitor == nullptr) {
        ammonite::utils::warning << "Can't set fullscreen monitor with a null pointer" \
                                 << std::endl;
        return;
      }

      internal::setFullscreenMonitor(monitor);
    }

    void setFullscreen(bool fullscreen) {
      internal::setFullscreen(fullscreen);
    }
  }
}
