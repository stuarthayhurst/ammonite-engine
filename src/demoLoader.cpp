#include <cstdlib>
#include <format>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <ammonite/ammonite.hpp>
#include <glm/glm.hpp>

#include "helper/argHandler.hpp"
#include "demos/object-field.hpp"
#include "demos/many-cubes.hpp"
#include "demos/monkey.hpp"
#include "demos/sponza.hpp"

//NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define EXPAND_DEMO(DEMO_NAME, NAMESPACE) {std::string(DEMO_NAME), \
  {NAMESPACE::preRendererInit, NAMESPACE::postRendererInit, \
   NAMESPACE::rendererMainloop, NAMESPACE::demoExit}}
//NOLINTEND(cppcoreguidelines-macro-usage)

//Definitions for demo loading
namespace {
  using DemoFunctionType = bool (*)();
  struct DemoFunctions {
    DemoFunctionType preRendererInit;
    DemoFunctionType postRendererInit;
    DemoFunctionType rendererMainloop;
    DemoFunctionType demoExit;
  };

  std::unordered_map<std::string, DemoFunctions> demoFunctionMap = {
    EXPAND_DEMO("object-field", objectFieldDemo),
    EXPAND_DEMO("many-cubes", manyCubesDemo),
    EXPAND_DEMO("monkey", monkeyDemo),
    EXPAND_DEMO("sponza", sponzaDemo)
  };
}

//Callbacks and definitions
namespace {
  struct CameraData {
    int cameraIndex;
    std::vector<AmmoniteId> cameraIds;
  };

  enum : unsigned char {
    HAS_SETUP_WINDOW   = (1 << 0),
    HAS_SETUP_RENDERER = (1 << 1),
    HAS_SETUP_CONTROLS = (1 << 2)
  };

  void inputFocusCallback(const std::vector<AmmoniteKeycode>&, KeyStateEnum, void*) {
    ammonite::input::setInputFocus(!ammonite::input::getInputFocus());
  }

  void fullscreenToggleCallback(const std::vector<AmmoniteKeycode>&, KeyStateEnum, void*) {
    ammonite::window::setFullscreen(!ammonite::window::getFullscreen());
  }

  void focalToggleCallback(const std::vector<AmmoniteKeycode>&, KeyStateEnum, void*) {
    ammonite::renderer::settings::post::setFocalDepthEnabled(
      !ammonite::renderer::settings::post::getFocalDepthEnabled());
  }

  void sprintToggleCallback(const std::vector<AmmoniteKeycode>&, KeyStateEnum action, void*) {
    float movementSpeed = 0.0f;
    if (action == AMMONITE_REPEAT) {
      return;
    }

    movementSpeed = ammonite::controls::settings::getMovementSpeed();
    movementSpeed *= (action == AMMONITE_PRESSED) ? 2.0f : (1.0f / 2.0f);
    ammonite::controls::settings::setMovementSpeed(movementSpeed);
  }

  void cameraCycleCallback(const std::vector<AmmoniteKeycode>&, KeyStateEnum, void* userPtr) {
    CameraData* cameraData = (CameraData*)userPtr;
    cameraData->cameraIndex = (int)((cameraData->cameraIndex + 1) % cameraData->cameraIds.size());
    ammonite::camera::setActiveCamera(cameraData->cameraIds[cameraData->cameraIndex]);
  }

  void changeFocalDepthCallback(const std::vector<AmmoniteKeycode>&,
                                KeyStateEnum action, void* userPtr) {
    static ammonite::utils::Timer focalDepthTimer;
    if (action != AMMONITE_RELEASED) {
      const float sign = *(float*)userPtr;
      const float unitsPerSecond = 0.8f;
      const float focalTimeDelta = (float)focalDepthTimer.getTime();
      float depth = ammonite::renderer::settings::post::getFocalDepth();

      depth += focalTimeDelta * unitsPerSecond * sign;
      ammonite::renderer::settings::post::setFocalDepth(depth);
      focalDepthTimer.unpause();
    } else {
      focalDepthTimer.pause();
    }
    focalDepthTimer.reset();
  }

  void changeFrameRateCallback(const std::vector<AmmoniteKeycode>&,
                               KeyStateEnum action, void* userPtr) {
    static ammonite::utils::Timer frameRateTimer;
    if (action != AMMONITE_RELEASED) {
      const float sign = *(float*)userPtr;
      const float unitsPerSecond = 10.0f;
      const float frameTimeDelta = (float)frameRateTimer.getTime();
      float frameRate = ammonite::renderer::settings::getFrameLimit();

      frameRate += frameTimeDelta * unitsPerSecond * sign;
      ammonite::utils::status << "Set framerate to " << frameRate << " fps" << std::endl;
      ammonite::renderer::settings::setFrameLimit(frameRate);
      frameRateTimer.unpause();
    } else {
      frameRateTimer.pause();
    }
    frameRateTimer.reset();
  }

  void closeWindowCallback(const std::vector<AmmoniteKeycode>&, KeyStateEnum, void* userPtr) {
    *(bool*)userPtr = true;
  }
}

//Helpers
namespace {
  std::string formatMetrics(double frameTime) {
    double frameRate = 0.0;
    if (frameTime != 0.0) {
      frameRate = 1 / frameTime;
    }

    return std::format("{:.2f} fps ({:f}ms)", frameRate, frameTime * 1000);
  }

  //Clean up anything that was created
  void cleanEngine(unsigned char setupBits, std::vector<AmmoniteId>* keybindIdsPtr) {
    if ((setupBits & HAS_SETUP_CONTROLS) != 0) {
      ammonite::controls::releaseFreeCamera();
      if (keybindIdsPtr != nullptr) {
        for (unsigned int i = 0; i < keybindIdsPtr->size(); i++) {
          ammonite::input::unregisterKeybind((*keybindIdsPtr)[i]);
        }
      }
    }

    if ((setupBits & HAS_SETUP_RENDERER) != 0) {
      ammonite::renderer::setup::destroyRenderer();
    }

    if ((setupBits & HAS_SETUP_WINDOW) != 0) {
      ammonite::window::destroyWindow();
    }
  }
}

int main(int argc, char** argv) noexcept(false) {
  //Handle arguments
  argc--;
  argv++;
  const int showHelp = arguments::searchArgument(argc, argv, "--help", nullptr);
  if (showHelp == 1) {
    std::cout << "Program help: \n"
    " --help      :  Display this help page\n"
    " --benchmark :  Start a benchmark\n"
    " --vsync     :  Enable / disable VSync (true / false)\n"
    " --demo      :  Run the selected demo" << std::endl;
    return EXIT_SUCCESS;
  }

  if (showHelp == -1) {
    return EXIT_FAILURE;
  }

  const bool useBenchmark = arguments::searchArgument(argc, argv, "--benchmark", nullptr) != 0;

  std::string useVsync;
  if (arguments::searchArgument(argc, argv, "--vsync", &useVsync) == -1) {
    std::cerr << "--vsync requires a value" << std::endl;
    return EXIT_FAILURE;
  }

  //Fetch demo, list options and validate the chosen demo
  std::string demoName;
  if (arguments::searchArgument(argc, argv, "--demo", &demoName) != 0) {
    if (demoName.empty()) {
      std::cout << "Valid demos:" << std::endl;
      for (auto it = demoFunctionMap.begin(); it != demoFunctionMap.end(); it++) {
        std::cout << " - " << it->first << std::endl;
      }

      return EXIT_SUCCESS;
    }
  }

  if (!demoFunctionMap.contains(demoName)) {
    if (!demoName.empty()) {
      ammonite::utils::warning << "Invalid demo '" << demoName << "', using default instead" \
                               << std::endl;
    }
    demoName = std::string("object-field");
  }

  DemoFunctionType preRendererInit = demoFunctionMap[demoName].preRendererInit;
  DemoFunctionType postRendererInit = demoFunctionMap[demoName].postRendererInit;
  DemoFunctionType rendererMainloop = demoFunctionMap[demoName].rendererMainloop;
  DemoFunctionType demoExit = demoFunctionMap[demoName].demoExit;

  //Start timer for demo loading
  ammonite::utils::Timer utilityTimer;

#ifdef AMMONITE_DEBUG
  ammonite::renderer::setup::requestContextType(AMMONITE_DEBUG_CONTEXT);
#elifdef FAST
  ammonite::renderer::setup::requestContextType(AMMONITE_NO_ERROR_CONTEXT);
#endif

  //Setup window and icon
  if (!ammonite::window::createWindow(1024, 768, "Ammonite Engine")) {
    return EXIT_FAILURE;
  }
  unsigned char setupBits = HAS_SETUP_WINDOW;
  ammonite::window::useIconDir("assets/icons/");

  ammonite::utils::debug::printDriverInfo();

  //Set vsync (disable if benchmarking)
  if (useVsync == "false" || useBenchmark) {
    ammonite::renderer::settings::setVsync(false);
  } else if (useVsync == "true") {
    ammonite::renderer::settings::setVsync(true);
  }

#ifdef AMMONITE_DEBUG
  ammonite::utils::debug::enableDebug();
#endif

  //Call pre-renderer demo setup
  if (preRendererInit != nullptr) {
    preRendererInit();
  }

  //Enable engine caching
  ammonite::utils::files::useDataCache("cache/");

  //Initialise renderer, clean up and exit on failure
  if (!ammonite::renderer::setup::setupRenderer("shaders/")) {
    ammonite::utils::error << "Failed to initialise renderer, exiting" << std::endl;
    cleanEngine(setupBits, nullptr);
    return EXIT_FAILURE;
  }
  setupBits |= HAS_SETUP_RENDERER;

  //Graphics settings
  ammonite::renderer::settings::setAntialiasingSamples(4);
  ammonite::renderer::settings::setGammaCorrection(true);
  ammonite::renderer::settings::setRenderResMultiplier(1.0f);
  ammonite::renderer::settings::setShadowRes(1024);
  ammonite::renderer::settings::setFrameLimit(0.0f);

  //Create a splash screen and render initial frame
  const AmmoniteId screenId = ammonite::splash::createSplashScreen();
  ammonite::splash::setActiveSplashScreen(screenId);
  ammonite::splash::setSplashScreenProgress(screenId, 0.0f);
  ammonite::renderer::drawFrame();

  //Call main demo setup
  if (postRendererInit != nullptr) {
    if (!postRendererInit()) {
      ammonite::utils::error << "Failed to set up demo, exiting" << std::endl;
      ammonite::splash::deleteSplashScreen(screenId);
      cleanEngine(setupBits, nullptr);
      return EXIT_FAILURE;
    }
  }

  //Camera data store
  CameraData cameraData;
  cameraData.cameraIndex = 0;
  cameraData.cameraIds = {1, ammonite::camera::createCamera()};
  ammonite::controls::setupFreeCamera(AMMONITE_W, AMMONITE_S,
    AMMONITE_SPACE, AMMONITE_LEFT_SHIFT, AMMONITE_D, AMMONITE_A);

  //Set the non-default camera to the start position
  ammonite::camera::setPosition(cameraData.cameraIds[1], glm::vec3(0.0f, 0.0f, 2.0f));

  //Set keybinds
  std::vector<AmmoniteId> keybindIds;
  keybindIds.push_back(ammonite::input::registerToggleKeybind(
                       AMMONITE_C, AMMONITE_ALLOW_OVERRIDE, inputFocusCallback, nullptr));
  keybindIds.push_back(ammonite::input::registerToggleKeybind(
                       AMMONITE_F11, AMMONITE_ALLOW_OVERRIDE,
                       fullscreenToggleCallback, nullptr));
  keybindIds.push_back(ammonite::input::registerToggleKeybind(
                       AMMONITE_Z, focalToggleCallback, nullptr));
  keybindIds.push_back(ammonite::input::registerKeybind(
                       AMMONITE_LEFT_CONTROL, sprintToggleCallback, nullptr));
  keybindIds.push_back(ammonite::input::registerToggleKeybind(
                       AMMONITE_B, cameraCycleCallback, &cameraData));

  //Set keybinds to adjust focal depth
  float positive = 1.0f;
  float negative = -1.0f;
  keybindIds.push_back(ammonite::input::registerKeybind(
                       AMMONITE_RIGHT_BRACKET, changeFocalDepthCallback, &positive));
  keybindIds.push_back(ammonite::input::registerKeybind(
                       AMMONITE_LEFT_BRACKET, changeFocalDepthCallback, &negative));

  //Set keybinds to adjust framerate limit
  keybindIds.push_back(ammonite::input::registerKeybind(
                       AMMONITE_UP, changeFrameRateCallback, &positive));
  keybindIds.push_back(ammonite::input::registerKeybind(
                       AMMONITE_DOWN, changeFrameRateCallback, &negative));
  setupBits |= HAS_SETUP_CONTROLS;

  //Set keybind for closing window
  bool closeWindow = false;
  keybindIds.push_back(ammonite::input::registerToggleKeybind(AMMONITE_ESCAPE,
                       AMMONITE_ALLOW_OVERRIDE, closeWindowCallback, &closeWindow));

  //Engine loaded, delete the splash screen
  ammonite::utils::status << "Loaded demo in " << utilityTimer.getTime() << "s\n" << std::endl;
  ammonite::splash::deleteSplashScreen(screenId);

  //Create and reset timers for performance metrics
  utilityTimer.reset();
  ammonite::utils::Timer frameTimer;

  //Draw frames until window closed
  while(!closeWindow && !ammonite::window::shouldWindowClose()) {
    //Every second, output the framerate
    if (frameTimer.getTime() >= 1.0f) {
      ammonite::utils::status << formatMetrics(ammonite::renderer::getFrameTime()) << std::endl;
      frameTimer.reset();
    }

    //Process new input since last frame
    ammonite::input::updateInput();

    //Call demo-specific main loop code
    if (rendererMainloop != nullptr) {
      if (!rendererMainloop()) {
        ammonite::utils::error << "Failed to run mainloop, exiting" << std::endl;
        cleanEngine(setupBits, &keybindIds);
        return EXIT_FAILURE;
      }
    }
  }

  //Output benchmark score
  if (useBenchmark) {
    const double frameTime = utilityTimer.getTime() / (double)ammonite::renderer::getTotalFrames();
    ammonite::utils::status.printEmptyLine();
    ammonite::utils::status << "Benchmark complete:" << std::endl;
    ammonite::utils::status << "  Average fps: ";
    ammonite::utils::status << formatMetrics(frameTime) << std::endl;
  }

  //Clean up and exit
  bool cleanExit = true;
  if (demoExit != nullptr) {
    if (!demoExit()) {
      cleanExit = false;
      ammonite::utils::error << "Failed to clean up, exiting" << std::endl;
    }
  }

  cleanEngine(setupBits, &keybindIds);
  if (!cleanExit) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
