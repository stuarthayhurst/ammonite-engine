#include <cmath>
#include <cstdlib>
#include <format>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <ammonite/ammonite.hpp>

#include "helper/argHandler.hpp"
#include "helper/commands.hpp"
#include "helper/modelPlacementMode.hpp"

#include "demos/object-field.hpp"
#include "demos/many-cubes.hpp"
#include "demos/monkey.hpp"
#include "demos/sponza.hpp"

//NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define EXPAND_DEMO(DEMO_NAME, NAMESPACE) {std::string(DEMO_NAME), \
  {NAMESPACE::preEngineInit, NAMESPACE::postEngineInit, \
   NAMESPACE::rendererMainloop, NAMESPACE::demoExit}}
//NOLINTEND(cppcoreguidelines-macro-usage)

//Definitions for demo loading
namespace {
  using DemoFunction = bool (*)();
  struct DemoFunctions {
    DemoFunction preEngineInit;
    DemoFunction postEngineInit;
    DemoFunction rendererMainloop;
    DemoFunction demoExit;
  };

  DemoFunction preEngineInit = nullptr;
  DemoFunction postEngineInit = nullptr;
  DemoFunction rendererMainloop = nullptr;
  DemoFunction demoExit = nullptr;

  std::unordered_map<std::string, DemoFunctions> demoFunctionMap = {
    EXPAND_DEMO("object-field", objectFieldDemo),
    EXPAND_DEMO("many-cubes", manyCubesDemo),
    EXPAND_DEMO("monkey", monkeyDemo),
    EXPAND_DEMO("sponza", sponzaDemo)
  };
}

//Helpers for demo loading
namespace {
  bool setDemo(int argc, char** argv) {
    //Fetch demo, list options and validate the chosen demo
    std::string demoName;
    if (arguments::searchArgument(argc, argv, "--demo", &demoName) != 0) {
      if (demoName.empty()) {
        std::cout << "Valid demos:" << std::endl;
        for (const auto& demoEntry : demoFunctionMap) {
          std::cout << " - " << demoEntry.first << std::endl;
        }

        return false;
      }
    }

    if (!demoFunctionMap.contains(demoName)) {
      if (!demoName.empty()) {
        ammonite::utils::warning << "Invalid demo '" << demoName << "', using default instead" \
                                 << std::endl;
      }

      demoName = std::string("object-field");
    }

    preEngineInit = demoFunctionMap[demoName].preEngineInit;
    postEngineInit = demoFunctionMap[demoName].postEngineInit;
    rendererMainloop = demoFunctionMap[demoName].rendererMainloop;
    demoExit = demoFunctionMap[demoName].demoExit;

    return true;
  }
}

//Callbacks and definitions
namespace {
  struct CameraData {
    int cameraIndex;
    std::vector<AmmoniteId> cameraIds;
  };

  enum : unsigned char {
    HAS_SETUP_ENGINE    = (1 << 0),
    HAS_SETUP_CONTROLS  = (1 << 1),
    HAS_SETUP_PATHS     = (1 << 2),
    HAS_SETUP_PLACEMENT = (1 << 3)
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
    CameraData* const cameraData = (CameraData*)userPtr;
    cameraData->cameraIndex = (int)((cameraData->cameraIndex + 1) % cameraData->cameraIds.size());
    ammonite::camera::setActiveCamera(cameraData->cameraIds[cameraData->cameraIndex]);
  }

  void pathRecordToggleCallback(const std::vector<AmmoniteKeycode>&,
                                KeyStateEnum action, void* userPtr) {
    AmmoniteId* const pathIdPtr = (AmmoniteId*)userPtr;

    //Create and register path to record
    if (action == AMMONITE_PRESSED) {
      if (*pathIdPtr == 0) {
        *pathIdPtr = ammonite::camera::path::createCameraPath();
        commands::registerCameraPath(*pathIdPtr);
      } else {
        *pathIdPtr = 0;
      }
    }
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

  void enableCommandPromptCallback(const std::vector<AmmoniteKeycode>&,
                                   KeyStateEnum, void* userPtr) {
    bool* const commandsEnabledPtr = (bool*)userPtr;
    *commandsEnabledPtr = true;
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
    if ((setupBits & HAS_SETUP_PLACEMENT) != 0) {
      placement::unsetPlacementCallbacks();
      placement::deletePlacedModels();
    }

    if ((setupBits & HAS_SETUP_PATHS) != 0) {
      commands::deleteCameraPaths();
    }

    if ((setupBits & HAS_SETUP_CONTROLS) != 0) {
      ammonite::controls::releaseFreeCamera();
      if (keybindIdsPtr != nullptr) {
        for (const AmmoniteId& keybindId : *keybindIdsPtr) {
          ammonite::input::unregisterKeybind(keybindId);
        }
      }
    }

    if ((setupBits & HAS_SETUP_ENGINE) != 0) {
      ammonite::destroyEngine();
    }
  }

  void recordCameraPathNode(AmmoniteId cameraPathId,
                            const ammonite::utils::Timer& pathTimer) {
    ammonite::Vec<float, 3> cameraPosition = {0};
    const AmmoniteId activeCameraId = ammonite::camera::getActiveCamera();

    ammonite::camera::getPosition(activeCameraId, cameraPosition);
    const double horizontal = ammonite::camera::getHorizontal(activeCameraId);
    const double vertical = ammonite::camera::getVertical(activeCameraId);

    ammonite::camera::path::addPathNode(cameraPathId, cameraPosition,
                                        horizontal, vertical, pathTimer.getTime());
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

  //Set the demo function pointers, exit if we just listed the demos
  if (!setDemo(argc, argv)) {
    return EXIT_SUCCESS;
  }

  //Start timer for demo loading
  ammonite::utils::Timer utilityTimer;

  //Call pre-renderer demo setup
  if (preEngineInit != nullptr) {
    preEngineInit();
  }

#ifdef AMMONITE_DEBUG
  ammonite::renderer::setup::requestContextType(AMMONITE_DEBUG_CONTEXT);
#elifdef AMMONITE_FAST
  ammonite::renderer::setup::requestContextType(AMMONITE_NO_ERROR_CONTEXT);
#endif

  //Enable engine caching
  ammonite::utils::files::useDataCache("cache/");

  //Initialise the engine
  unsigned char setupBits = HAS_SETUP_ENGINE;
  if (!ammonite::setupEngine("shaders/", 1024, 768, "Ammonite Engine")) {
    ammonite::utils::error << "Failed to initialise engine, exiting" << std::endl;
    return EXIT_FAILURE;
  }
  ammonite::window::useIconDir("assets/icons/");

  //Set VSync according to arguments
  if (useVsync == "false" || useBenchmark) {
    ammonite::renderer::settings::setVsync(false);
  } else if (useVsync == "true") {
    ammonite::renderer::settings::setVsync(true);
  }

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

  //Camera data store
  CameraData cameraData;
  cameraData.cameraIndex = 0;
  cameraData.cameraIds = {1, ammonite::camera::createCamera()};
  ammonite::controls::setupFreeCamera(AMMONITE_W, AMMONITE_S,
    AMMONITE_SPACE, AMMONITE_LEFT_SHIFT, AMMONITE_D, AMMONITE_A);

  //Set up the camera path
  const unsigned int nodeCount = 100 + 1;
  const double pathDuration = 6.0;
  const float pathRadius = 16.0;
  const float pathHeight = 12.0f;
  const AmmoniteId pathId = ammonite::camera::path::createCameraPath(nodeCount);

  //Generate and add the path's nodes
  for (unsigned int i = 0; i < nodeCount; i++) {
    ammonite::Vec<float, 3> position = {0};
    ammonite::Vec<float, 3> direction = {0};

    //Calculate the node's coordinates
    const double progress = (double)(i) / (double)(nodeCount - 1);
    const float angle = (float)progress * ammonite::twoPi<float>;
    position[0] = pathRadius * std::sin(angle);
    position[1] = pathHeight;
    position[2] = pathRadius * std::cos(angle);

    //Look towards the origin
    ammonite::normalise(ammonite::negate(position, direction));

    const double time = progress * pathDuration;
    ammonite::camera::path::addPathNode(pathId, position, direction, time);
  }

  //Start the path for the non-default camera and enable looping
  ammonite::camera::path::setPathMode(pathId, AMMONITE_PATH_LOOP);
  ammonite::camera::setLinkedPath(cameraData.cameraIds[1], pathId);
  ammonite::camera::path::playPath(pathId);

  //Register the path with the command system
  commands::registerCameraPath(pathId);
  setupBits |= HAS_SETUP_PATHS;

  //Set up model placement mode
  placement::resetPlacementDistance();
  placement::setPlacementCallbacks();
  setupBits |= HAS_SETUP_PLACEMENT;

  //Set keybinds
  std::vector<AmmoniteId> keybindIds;
  bool commandPromptRequested = false;
  AmmoniteId recordingCameraPathId = 0;
  keybindIds.push_back(ammonite::input::registerToggleKeybind(
                       AMMONITE_C, AMMONITE_ALLOW_OVERRIDE, inputFocusCallback, nullptr));
  keybindIds.push_back(ammonite::input::registerToggleKeybind(
                       AMMONITE_V, AMMONITE_ALLOW_OVERRIDE,
                       enableCommandPromptCallback, &commandPromptRequested));
  keybindIds.push_back(ammonite::input::registerToggleKeybind(
                       AMMONITE_F11, AMMONITE_ALLOW_OVERRIDE,
                       fullscreenToggleCallback, nullptr));
  keybindIds.push_back(ammonite::input::registerToggleKeybind(
                       AMMONITE_Z, focalToggleCallback, nullptr));
  keybindIds.push_back(ammonite::input::registerKeybind(
                       AMMONITE_LEFT_CONTROL, sprintToggleCallback, nullptr));
  keybindIds.push_back(ammonite::input::registerToggleKeybind(
                       AMMONITE_B, cameraCycleCallback, &cameraData));
  keybindIds.push_back(ammonite::input::registerToggleKeybind(
                       AMMONITE_G, pathRecordToggleCallback, &recordingCameraPathId));

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

  //Call main demo setup
  if (postEngineInit != nullptr) {
    if (!postEngineInit()) {
      ammonite::utils::error << "Failed to set up demo, exiting" << std::endl;
      ammonite::splash::deleteSplashScreen(screenId);
      cleanEngine(setupBits, nullptr);
      return EXIT_FAILURE;
    }
  }

  //Engine loaded, delete the splash screen
  ammonite::utils::status << "Loaded demo in " << utilityTimer.getTime() << "s\n" << std::endl;
  ammonite::splash::deleteSplashScreen(screenId);

  //Create and reset timers for performance metrics
  utilityTimer.reset();
  ammonite::utils::Timer secondTimer;
  ammonite::utils::Timer pathTimer(false);

  //Draw frames until window closed
  while(!closeWindow && !ammonite::window::shouldWindowClose()) {
    //Update the engine timer for the new frame
    ammonite::updateFrameTime();
    ammonite::camera::path::updatePathProgress();

    //Every second, output the framerate
    if (secondTimer.getTime() >= 1.0f) {
      ammonite::utils::status << formatMetrics(ammonite::renderer::getAverageFrameTime()) << std::endl;
      secondTimer.reset();
    }

    //Process new input since last frame
    ammonite::input::updateInput();

    //Swap to command prompt if requested
    if (commandPromptRequested) {
      //Release input focus
      const bool hadInputFocus = ammonite::input::getInputFocus();
      ammonite::input::setInputFocus(false);

      if (commands::commandPrompt()) {
        //Exit if requested
        closeWindow = true;
      }

      //Swap input back to window
      ammonite::input::setInputFocus(hadInputFocus);
      commandPromptRequested = false;
    }

    //Camera path recording
    if (recordingCameraPathId != 0) {
      //Start the path timer, if required
      if (!pathTimer.isRunning()) {
        pathTimer.unpause();
        pathTimer.reset();

        ammonite::utils::status << "Recording camera path" << std::endl;
      }

      recordCameraPathNode(recordingCameraPathId, pathTimer);
    } else {
      //Stop the path timer
      if (pathTimer.isRunning()) {
        pathTimer.pause();

        ammonite::utils::status << "Finished recording camera path" << std::endl;
      }
    }

    //Handle object placement mode
    placement::updatePlacementPosition();

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
