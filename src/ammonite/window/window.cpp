#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

extern "C" {
  #include <GLFW/glfw3.h>
  #include <stb/stb_image.h>
}

#include "window.hpp"

#include "../enums.hpp"
#include "../input/input.hpp"
#include "../utils/logging.hpp"

static constexpr std::string DEFAULT_TITLE = "Ammonite Window";

namespace ammonite {
  namespace window {
    namespace {
      GLFWwindow* windowPtr = nullptr;
      AmmoniteContextEnum requestedContextType = AMMONITE_DEFAULT_CONTEXT;
    }

    namespace internal {
      GLFWwindow* getWindowPtr() {
        return windowPtr;
      }

      void setContextType(AmmoniteContextEnum contextType) {
        if (windowPtr != nullptr) {
          ammonite::utils::warning << "Window already created, ignoring context type request" << std::endl;
          return;
        }

        requestedContextType = contextType;
      }
    }

    bool createWindow(unsigned int width, unsigned int height, const std::string& title) {
      //Setup GLFW
      if (!internal::setupGlfw(requestedContextType)) {
        ammonite::utils::error << "Failed to initialise GLFW" << std::endl;
        return false;
      }

      //Create window
      windowPtr = internal::createWindow(width, height);
      if (windowPtr == nullptr) {
        ammonite::utils::error << "Failed to open window" << std::endl;
        return false;
      }

      //Set window title
      setTitle(title);

      //Setup input for window
      ammonite::input::internal::setupInputCallback(windowPtr);
      ammonite::input::internal::setupMouseCallback(windowPtr);
      internal::setFocusCallback(windowPtr);

      return true;
    }

    bool createWindow(unsigned int width, unsigned int height) {
      return createWindow(width, height, DEFAULT_TITLE);
    }

    void destroyWindow() {
      internal::destroyGlfw();
    }

    void setWindowResizable(bool resizable) {
      if (windowPtr == nullptr) {
        ammonite::utils::warning << "No window to set resizability for" << std::endl;
        return;
      }

      internal::setWindowResizable(windowPtr, resizable);
    }

    bool getWindowResizable() {
      if (windowPtr == nullptr) {
        ammonite::utils::warning << "No window to get resizability for" << std::endl;
        return false;
      }

      return internal::getWindowResizable(windowPtr);
    }

    void setTitle(const std::string& title) {
      if (windowPtr == nullptr) {
        ammonite::utils::warning << "No window to set icon for" << std::endl;
        return;
      }

      internal::setTitle(windowPtr, title);
    }

    void useIcons(const std::string* iconPaths, unsigned int iconCount) {
      if (iconPaths == nullptr || iconCount == 0) {
        ammonite::utils::warning << "Failed to load icons (none specified)" << std::endl;
        return;
      }

      //Read image data
      internal::ImageData* images = new internal::ImageData[iconCount];
      for (unsigned int i = 0; i < iconCount; i++) {
        int width = 0, height = 0;
        images[i].data = stbi_load(iconPaths[i].c_str(), &width, &height, nullptr, 4);
        images[i].width = (unsigned int)width;
        images[i].height = (unsigned int)height;

        if (images[i].data == nullptr) {
          ammonite::utils::warning << "Failed to load '" << iconPaths[i] << "'" << std::endl;

          //Free already loaded images
          for (unsigned int j = 0; j < i; j++) {
            stbi_image_free(images[j].data);
          }
          delete [] images;
          return;
        }
      }

      //Pass icons to window implementation
      internal::setIcons(windowPtr, images, iconCount);

      //Free the data
      for (unsigned int i = 0; i < iconCount; i++) {
        stbi_image_free(images[i].data);
      }
      delete [] images;
    }

    void useIcon(const std::string& iconPath) {
      useIcons(&iconPath, 1);
    }

    void useIconDir(const std::string& iconDirPath) {
      //Attempt to add all PNG files to a vector
      std::vector<std::string> pngFiles(0);
      try {
        const std::filesystem::path& searchDir{iconDirPath};
        const auto it = std::filesystem::directory_iterator{searchDir};

        for (auto const& fileName : it) {
          const std::filesystem::path& filePath{fileName};
          if (filePath.extension() == ".png") {
            pngFiles.push_back(std::string(filePath));
          }
        }
      } catch (const std::filesystem::filesystem_error&) {
        ammonite::utils::warning << "Couldn't open '" << iconDirPath << "'" << std::endl;
        return;
      }

      //Hand off to another icon handler
      useIcons(pngFiles.data(), pngFiles.size());
    }

    //Set decorated window size and position, for non-fullscreen windows only
    void setWindowGeometry(unsigned int width, unsigned int height, unsigned int xPos,
                           unsigned int yPos, bool useDecoratedSize, bool useDecoratedPos) {
      if (windowPtr == nullptr) {
        ammonite::utils::warning << "No window to set geometry for" << std::endl;
        return;
      }

      //Don't allow setting window geometry for fullscreen windows
      if (getFullscreen()) {
        ammonite::utils::warning << "Ignoring window geometry request for fullscreen window" \
                                 << std::endl;
        return;
      }

      internal::setWindowGeometry(windowPtr, width, height, xPos, yPos, useDecoratedSize, useDecoratedPos);
    }

    //Return geometry information for the active window
    void getWindowGeometry(unsigned int* width, unsigned int* height, unsigned int* xPos,
                           unsigned int* yPos, bool useDecoratedSize, bool useDecoratedPos) {
      if (windowPtr == nullptr) {
        ammonite::utils::warning << "No window to get geometry for" << std::endl;
        return;
      }

      unsigned int ignore = 0;
      internal::getWindowGeometry(windowPtr, (width != nullptr) ? width : &ignore,
                                  (height != nullptr) ? height : &ignore,
                                  (xPos != nullptr) ? xPos : &ignore,
                                  (yPos != nullptr) ? yPos : &ignore,
                                  useDecoratedSize, useDecoratedPos);
    }

    /*
     - Change the current monitor for a fullscreen window
     - Does nothing if the window isn't fullscreen
    */
    void changeFullscreenMonitor(unsigned int monitorIndex) {
      if (windowPtr == nullptr) {
        ammonite::utils::warning << "Window system hasn't been initialised" << std::endl;
        return;
      }

      //Ignore requests for non-fullscreen windows
      if (!getFullscreen()) {
        ammonite::utils::warning << "Can't set monitor for non-fullscreen window" << std::endl;
        return;
      }

      internal::setFullscreenMonitor(windowPtr, monitorIndex);
    }

    /*
     - Make a window fullscreen, or restore a fullscreen window to windowed
     - Windowing a fullscreen window attempts to restore the previous geometry
    */
    void setFullscreen(bool shouldFullscreen, unsigned int monitorIndex) {
      if (windowPtr == nullptr) {
        ammonite::utils::warning << "Window system hasn't been initialised" << std::endl;
        return;
      }

      //Ignore request if the state already matches
      if (getFullscreen() == shouldFullscreen) {
        return;
      }

      internal::setFullscreen(windowPtr, shouldFullscreen, monitorIndex);
    }

    /*
     - Same as the more configurable setFullscreen(), without specifying a monitor
     - Instead, attempt to guess which monitor the window is on
    */
    void setFullscreen(bool shouldFullscreen) {
      if (windowPtr == nullptr) {
        ammonite::utils::warning << "Window system hasn't been initialised" << std::endl;
        return;
      }

      setFullscreen(shouldFullscreen, getCurrentMonitorIndex());
    }

    //Return the fullscreen monitor, or the closest matched when windowed
    unsigned int getCurrentMonitorIndex() {
      if (windowPtr == nullptr) {
        ammonite::utils::warning << "No window to get monitor for" << std::endl;
        return 0;
      }

      return internal::getCurrentMonitorIndex(windowPtr);
    }

    /*
     - Return the number of monitors
     - The highest monitor index will be 1 less than this
    */
    unsigned int getMonitorCount() {
      if (windowPtr == nullptr) {
        ammonite::utils::warning << "Window system hasn't been initialised" << std::endl;
        return 0;
      }

      return internal::getMonitorCount();
    }

    bool getFullscreen() {
      if (windowPtr == nullptr) {
        ammonite::utils::warning << "No window to get fullscreen state for" << std::endl;
        return false;
      }

      return internal::getFullscreen();
    }

    bool shouldWindowClose() {
      if (windowPtr == nullptr) {
        ammonite::utils::warning << "No window that could be closed" << std::endl;
        return false;
      }

      return internal::shouldWindowClose(windowPtr);
    }
  }
}
