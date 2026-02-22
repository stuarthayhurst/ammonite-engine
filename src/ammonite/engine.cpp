#include <iostream>
#include <string>
#include <string_view>

#include "engine.hpp"

#include "graphics/renderer.hpp"
#include "utils/debug.hpp"
#include "utils/logging.hpp"
#include "utils/thread.hpp"
#include "utils/timer.hpp"
#include "window/window.hpp"

#define MACRO_STRING(value) #value
//NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EXPAND_MACRO_STRING(macro) MACRO_STRING(macro)

namespace ammonite {
  namespace {
    const char* engineName = "Ammonite Engine";
    const char* engineVersion = EXPAND_MACRO_STRING(AMMONITE_VERSION);
  }

  std::string_view getEngineName() {
    return engineName;
  }

  std::string_view getEngineVersion() {
    return engineVersion;
  }

  bool setupEngine(const std::string& shaderPath, unsigned int width,
                   unsigned int height, const std::string& title) {
    const ammonite::utils::Timer loadTimer;

    //Create a thread pool
    if (!ammonite::utils::thread::createThreadPool(0)) {
      ammonite::utils::error << "Failed to create thread pool" << std::endl;
      return false;
    }

    ammonite::utils::status << "Created thread pool with " \
                            << ammonite::utils::thread::getThreadPoolSize() \
                            << " thread(s)" << std::endl;

    if (!title.empty()) {
      if (!ammonite::window::createWindow(width, height, title)) {
        return false;
      }
    } else {
      if (!ammonite::window::createWindow(width, height)) {
        return false;
      }
    }

#ifdef AMMONITE_DEBUG
    ammonite::utils::debug::enableDebug();
#endif

    //Print driver / hardware information
    ammonite::utils::debug::printDriverInfo();

    if (!ammonite::renderer::setup::setupRenderer(shaderPath)) {
      ammonite::utils::error << "Failed to initialise renderer" << std::endl;
      ammonite::window::destroyWindow();
      return false;
    }

    ammonite::utils::status << "Loaded engine in " << loadTimer.getTime() << "s" << std::endl;
    return true;
  }

  bool setupEngine(const std::string& shaderPath, unsigned int width,
                   unsigned int height) {
    return setupEngine(shaderPath, width, height, "");
  }

  void destroyEngine() {
    ammonite::utils::thread::destroyThreadPool();
    ammonite::renderer::setup::destroyRenderer();
    ammonite::window::destroyWindow();
  }
}
