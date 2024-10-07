#include <iostream>
#include <cstdlib>
#include <map>
#include <print>
#include <string>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "ammonite/ammonite.hpp"
#include "helper/argHandler.hpp"

#include "demos/object-field.hpp"
#include "demos/many-cubes.hpp"
#include "demos/monkey.hpp"
#include "demos/sponza.hpp"

#define EXPAND_DEMO(DEMO_NAME, NAMESPACE) {std::string(DEMO_NAME), {NAMESPACE::preRendererInit,\
                              NAMESPACE::postRendererInit,\
                              NAMESPACE::rendererMainloop,\
                              NAMESPACE::demoExit}}

#define HAS_SETUP_WINDOW   (1 << 0)
#define HAS_SETUP_RENDERER (1 << 1)
#define HAS_SETUP_CONTROLS (1 << 2)

//Definitions for demo loading
namespace {
  typedef int (*DemoFunctionType)(void);
  struct DemoFunctions {
    DemoFunctionType preRendererInit;
    DemoFunctionType postRendererInit;
    DemoFunctionType rendererMainloop;
    DemoFunctionType demoExit;
  };

  std::map<std::string, DemoFunctions> demoFunctionMap = {
    EXPAND_DEMO("object-field", objectFieldDemo),
    EXPAND_DEMO("many-cubes", manyCubesDemo),
    EXPAND_DEMO("monkey", monkeyDemo),
    EXPAND_DEMO("sponza", sponzaDemo)
  };
}

//Struct definitions
namespace {
  struct CameraData {
    int cameraIndex;
    std::vector<AmmoniteId> cameraIds;
  };
}

//Callbacks
namespace {
  static void inputFocusCallback(std::vector<int>, int, void*) {
    ammonite::input::setInputFocus(!ammonite::input::getInputFocus());
  }

  static void fullscreenToggleCallback(std::vector<int>, int, void*) {
    ammonite::window::setFullscreen(!ammonite::window::getFullscreen());
  }

  static void focalToggleCallback(std::vector<int>, int, void*) {
    ammonite::renderer::settings::post::setFocalDepthEnabled(
      !ammonite::renderer::settings::post::getFocalDepthEnabled());
  }

  static void sprintToggleCallback(std::vector<int>, int action, void*) {
    float movementSpeed = 0.0f;
    if (action == GLFW_REPEAT) {
      return;
    }

    movementSpeed = ammonite::controls::settings::getMovementSpeed();
    movementSpeed *= (action == GLFW_PRESS) ? 2.0f : (1.0f / 2.0f);
    ammonite::controls::settings::setMovementSpeed(movementSpeed);
  }

  static void cameraCycleCallback(std::vector<int>, int, void* userPtr) {
    CameraData* cameraData = (CameraData*)userPtr;
    cameraData->cameraIndex = (int)((cameraData->cameraIndex + 1) % cameraData->cameraIds.size());
    ammonite::camera::setActiveCamera(cameraData->cameraIds[cameraData->cameraIndex]);
  }

  static void changeFocalDepthCallback(std::vector<int>, int action, void* userPtr) {
    static ammonite::utils::Timer focalDepthTimer;
    if (action != GLFW_RELEASE) {
      float sign = *(float*)userPtr;
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

  static void changeFrameRateCallback(std::vector<int>, int action, void* userPtr) {
    static ammonite::utils::Timer frameRateTimer;
    if (action != GLFW_RELEASE) {
      float sign = *(float*)userPtr;
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
}

//Helpers
namespace {
  static void printMetrics(double frameTime) {
    double frameRate = 0.0;
    if (frameTime != 0.0) {
      frameRate = 1 / frameTime;
    }

    std::print("{:.2f} fps", frameRate);
    std::println(" ({:f}ms)", frameTime * 1000);
  }

  //Clean up anything that was created
  static void cleanEngine(unsigned int setupBits, std::vector<AmmoniteId>* keybindIdsPtr) {
    if (setupBits & HAS_SETUP_CONTROLS) {
      ammonite::controls::releaseFreeCamera();
      if (keybindIdsPtr != nullptr) {
        for (unsigned int i = 0; i < keybindIdsPtr->size(); i++) {
          ammonite::input::unregisterKeybind((*keybindIdsPtr)[i]);
        }
      }
    }

    if (setupBits & HAS_SETUP_RENDERER) {
      ammonite::renderer::setup::destroyRenderer();
    }

    if (setupBits & HAS_SETUP_WINDOW) {
      ammonite::window::destroyWindow();
    }
  }
}

int main(int argc, char* argv[]) {
  //Handle arguments
  const int showHelp = arguments::searchArgument(argc, argv, "--help", nullptr);
  if (showHelp == 1) {
    std::cout << "Program help: \n"
    " --help      :  Display this help page\n"
    " --benchmark :  Start a benchmark\n"
    " --vsync     :  Enable / disable VSync (true / false)\n"
    " --demo      :  Run the selected demo" << std::endl;
    return EXIT_SUCCESS;
  } else if (showHelp == -1) {
    return EXIT_FAILURE;
  }

  const bool useBenchmark = arguments::searchArgument(argc, argv, "--benchmark", nullptr);

  std::string useVsync;
  if (arguments::searchArgument(argc, argv, "--vsync", &useVsync) == -1) {
    std::cerr << "--vsync requires a value" << std::endl;
    return EXIT_FAILURE;
  }

  //Fetch demo, list options and validate the chosen demo
  std::string demoName;
  if (arguments::searchArgument(argc, argv, "--demo", &demoName) != 0) {
    if (demoName == "") {
      std::cout << "Valid demos:" << std::endl;
      for (auto it = demoFunctionMap.begin(); it != demoFunctionMap.end(); it++) {
        std::cout << " - " << it->first << std::endl;
      }

      return EXIT_SUCCESS;
    }
  }

  if (!demoFunctionMap.contains(demoName)) {
    if (demoName != "") {
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

#ifdef DEBUG
  ammonite::window::requestContextType(AMMONITE_DEBUG_CONTEXT);
#elifdef FAST
  ammonite::window::requestContextType(AMMONITE_NO_ERROR_CONTEXT);
#endif

  //Setup window and icon
  if (ammonite::window::createWindow(1024, 768, "Ammonite Engine") == -1) {
    return EXIT_FAILURE;
  }
  unsigned int setupBits = HAS_SETUP_WINDOW;
  ammonite::window::useIconDir("assets/icons/");

  ammonite::utils::debug::printDriverInfo();
  std::cout << std::endl;

  //Set vsync (disable if benchmarking)
  if (useVsync == "false" or useBenchmark) {
    ammonite::renderer::settings::setVsync(false);
  } else if (useVsync == "true") {
    ammonite::renderer::settings::setVsync(true);
  }

#ifdef DEBUG
  ammonite::utils::debug::enableDebug();
#endif

  //Call pre-renderer demo setup
  if (preRendererInit != nullptr) {
    preRendererInit();
  }

  //Enable engine caching, setup renderer and initialise controls
  bool success = true;
  ammonite::utils::files::useDataCache("cache/");
  ammonite::renderer::setup::setupRenderer("shaders/", &success);
  setupBits |= HAS_SETUP_RENDERER;

  //Graphics settings
  ammonite::renderer::settings::setAntialiasingSamples(4);
  ammonite::renderer::settings::setGammaCorrection(true);
  ammonite::renderer::settings::setRenderResMultiplier(1.0f);
  ammonite::renderer::settings::setShadowRes(1024);
  ammonite::renderer::settings::setFrameLimit(0.0f);

  //Renderer failed to initialise, clean up and exit
  if (!success) {
    ammonite::utils::error << "Failed to initialise renderer, exiting" << std::endl;
    cleanEngine(setupBits, nullptr);
    return EXIT_FAILURE;
  }

  //Create a loading screen and render initial frame
  AmmoniteId screenId = ammonite::interface::createLoadingScreen();
  ammonite::interface::setActiveLoadingScreen(screenId);
  ammonite::interface::setLoadingScreenProgress(screenId, 0.0f);
  ammonite::renderer::drawFrame();

  //Call main demo setup
  if (postRendererInit != nullptr) {
    if (postRendererInit() == -1) {
      ammonite::utils::error << "Failed to set up demo, exiting" << std::endl;
      ammonite::interface::deleteLoadingScreen(screenId);
      cleanEngine(setupBits, nullptr);
      return EXIT_FAILURE;
    }
  }

  //Camera data store
  CameraData cameraData;
  cameraData.cameraIndex = 0;
  cameraData.cameraIds = {1, ammonite::camera::createCamera()};
  ammonite::controls::setupFreeCamera(GLFW_KEY_W, GLFW_KEY_S,
    GLFW_KEY_SPACE, GLFW_KEY_LEFT_SHIFT, GLFW_KEY_D, GLFW_KEY_A);

  //Set the non-default camera to the start position
  ammonite::camera::setPosition(cameraData.cameraIds[1], glm::vec3(0.0f, 0.0f, 2.0f));

  //Set keybinds
  std::vector<AmmoniteId> keybindIds;
  keybindIds.push_back(ammonite::input::registerToggleKeybind(
                         GLFW_KEY_C, AMMONITE_ALLOW_OVERRIDE, inputFocusCallback, nullptr));
  keybindIds.push_back(ammonite::input::registerToggleKeybind(
                         GLFW_KEY_F11, AMMONITE_ALLOW_OVERRIDE,
                         fullscreenToggleCallback, nullptr));
  keybindIds.push_back(ammonite::input::registerToggleKeybind(
                         GLFW_KEY_Z, focalToggleCallback, nullptr));
  keybindIds.push_back(ammonite::input::registerKeybind(
                         GLFW_KEY_LEFT_CONTROL, sprintToggleCallback, nullptr));
  keybindIds.push_back(ammonite::input::registerToggleKeybind(
                         GLFW_KEY_B, cameraCycleCallback, &cameraData));

  //Set keybind for closing window
  keybindIds.push_back(ammonite::window::registerWindowCloseKeybind(GLFW_KEY_ESCAPE));

  //Set keybinds to adjust focal depth
  float positive = 1.0f;
  float negative = -1.0f;
  keybindIds.push_back(ammonite::input::registerKeybind(
                         GLFW_KEY_RIGHT_BRACKET, changeFocalDepthCallback, &positive));
  keybindIds.push_back(ammonite::input::registerKeybind(
                         GLFW_KEY_LEFT_BRACKET, changeFocalDepthCallback, &negative));

  //Set keybinds to adjust framerate limit
  keybindIds.push_back(ammonite::input::registerKeybind(
                         GLFW_KEY_UP, changeFrameRateCallback, &positive));
  keybindIds.push_back(ammonite::input::registerKeybind(
                         GLFW_KEY_DOWN, changeFrameRateCallback, &negative));
  setupBits |= HAS_SETUP_CONTROLS;

  //Engine loaded, delete the loading screen
  ammonite::utils::status << "Loaded demo in " << utilityTimer.getTime() << "s\n" << std::endl;
  ammonite::interface::deleteLoadingScreen(screenId);

  //Create and reset timers for performance metrics
  utilityTimer.reset();
  ammonite::utils::Timer frameTimer;

  //Draw frames until window closed
  while(!ammonite::window::shouldWindowClose()) {
    //Every second, output the framerate
    if (frameTimer.getTime() >= 1.0f) {
      printMetrics(ammonite::renderer::getFrameTime());
      frameTimer.reset();
    }

    //Process new input since last frame
    ammonite::input::updateInput();

    //Call demo-specific main loop code
    if (rendererMainloop != nullptr) {
      if (rendererMainloop() == -1) {
        ammonite::utils::error << "Failed to run mainloop, exiting" << std::endl;
        cleanEngine(setupBits, &keybindIds);
        return EXIT_FAILURE;
      }
    }
  }

  //Output benchmark score
  if (useBenchmark) {
    std::cout << "\nBenchmark complete:" << std::endl;
    std::cout << "  Average fps: ";
    printMetrics(utilityTimer.getTime() / (double)ammonite::renderer::getTotalFrames());
  }

  //Clean up and exit
  bool cleanExit = true;
  if (demoExit != nullptr) {
    if (demoExit() == -1) {
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
