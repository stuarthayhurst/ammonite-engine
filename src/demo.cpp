#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <map>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "ammonite/ammonite.hpp"
#include "helper/argHandler.hpp"

#include "demos/default.hpp"
#include "demos/object-field.hpp"
#include "demos/many-cubes.hpp"
#include "demos/sponza.hpp"

#define EXPAND_DEMO(DEMO_NAME, NAMESPACE) {std::string(DEMO_NAME), {NAMESPACE::preRendererInit,\
                              NAMESPACE::postRendererInit,\
                              NAMESPACE::rendererMainloop,\
                              NAMESPACE::demoExit}}

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
    EXPAND_DEMO("default", defaultDemo),
    EXPAND_DEMO("object-field", objectFieldDemo),
    EXPAND_DEMO("many-cubes", manyCubesDemo),
    EXPAND_DEMO("sponza", sponzaDemo)
  };
}

//Struct definitions
struct CameraData {
  int cameraIndex;
  std::vector<int> cameraIds;
};

//Callbacks
void inputFocusCallback(int, int, void*) {
  ammonite::input::setInputFocus(!ammonite::input::getInputFocus());
}

void fullscreenToggleCallback(int, int, void*) {
  ammonite::window::setFullscreen(!ammonite::window::getFullscreen());
}

void focalToggleCallback(int, int, void*) {
  ammonite::settings::graphics::post::setFocalDepthEnabled(
    !ammonite::settings::graphics::post::getFocalDepthEnabled());
}

void cameraCycleCallback(int, int, void* userPtr) {
  CameraData* cameraData = (CameraData*)userPtr;
  cameraData->cameraIndex = (cameraData->cameraIndex + 1) % cameraData->cameraIds.size();
  ammonite::camera::setActiveCamera(cameraData->cameraIds[cameraData->cameraIndex]);
}

void changeFocalDepthCallback(int, int action, void* userPtr) {
  static ammonite::utils::Timer focalDepthTimer;
  if (action != GLFW_RELEASE) {
    float sign = *(float*)userPtr;
    const float unitsPerSecond = 1.0f;
    const float focalTimeDelta = focalDepthTimer.getTime();
    float depth = ammonite::settings::graphics::post::getFocalDepth();

    depth += focalTimeDelta * unitsPerSecond * sign;
    ammonite::settings::graphics::post::setFocalDepth(depth);
    focalDepthTimer.unpause();
  } else {
    focalDepthTimer.pause();
  }
  focalDepthTimer.reset();
}

//Helpers
void printMetrics(double frameTime) {
  double frameRate = 0.0;
  if (frameTime != 0.0) {
    frameRate = 1 / frameTime;
  }

  std::printf("%.2f fps", frameRate);
  std::printf(" (%fms)\n", frameTime * 1000);
}

void cleanUp(int modelCount, std::vector<int> loadedModelIds) {
  for (int i = 0; i < modelCount; i++) {
    ammonite::models::deleteModel(loadedModelIds[i]);
  }
  ammonite::window::destroyWindow();
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
      std::cerr << "WARNING: Invalid demo '" << demoName << "', using default instead" << std::endl;
    }
    demoName = std::string("default");
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

  //Create the window
  if (ammonite::window::createWindow(1024, 768, "OpenGL Experiments") == -1) {
    return EXIT_FAILURE;
  }
  ammonite::window::getWindowPtr();

  //Set an icon
  ammonite::window::useIconDir("assets/icons/");

  //Set vsync (disable if benchmarking)
  if (useVsync == "false" or useBenchmark) {
    ammonite::settings::graphics::setVsync(false);
  } else if (useVsync == "true") {
    ammonite::settings::graphics::setVsync(true);
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
  ammonite::utils::cache::useDataCache("cache");
  ammonite::renderer::setup::setupRenderer("shaders/", &success);
  ammonite::utils::controls::setupControls();

  //Graphics settings
  ammonite::settings::graphics::setAntialiasingSamples(4);
  ammonite::settings::graphics::setGammaCorrection(true);
  ammonite::settings::graphics::setRenderResMultiplier(1.0f);
  ammonite::settings::graphics::setShadowRes(1024);
  ammonite::settings::graphics::setFrameLimit(0.0f);

  //Renderer failed to initialise, clean up and exit
  if (!success) {
    std::cerr << "ERROR: Failed to initialise renderer, exiting" << std::endl;
    ammonite::window::destroyWindow();
    return EXIT_FAILURE;
  }

  //Create a loading screen and render initial frame
  int screenId = ammonite::interface::createLoadingScreen();
  ammonite::interface::setActiveLoadingScreen(screenId);
  ammonite::interface::setLoadingScreenProgress(screenId, 0.0f);
  ammonite::renderer::drawFrame();

  //Call main demo setup
  if (postRendererInit != nullptr) {
    if (postRendererInit() == -1) {
      std::cerr << "ERROR: Failed to set up demo, exiting" << std::endl;
      ammonite::window::destroyWindow();
      return EXIT_FAILURE;
    }
  }

  //Camera data store
  CameraData cameraData;
  cameraData.cameraIndex = 0;
  cameraData.cameraIds = {0, ammonite::camera::createCamera()};

  //Set the camera to the start position
  ammonite::camera::setPosition(cameraData.cameraIds[0], glm::vec3(0.0f, 0.0f, 5.0f));
  ammonite::camera::setPosition(cameraData.cameraIds[1], glm::vec3(0.0f, 0.0f, 2.0f));

  //Set keybinds
  ammonite::input::registerToggleKeybind(GLFW_KEY_C, AMMONITE_ALLOW_OVERRIDE,
                                         inputFocusCallback, nullptr);
  ammonite::input::registerToggleKeybind(GLFW_KEY_F11, fullscreenToggleCallback, nullptr);
  ammonite::input::registerToggleKeybind(GLFW_KEY_Z, focalToggleCallback, nullptr);
  ammonite::input::registerToggleKeybind(GLFW_KEY_B, cameraCycleCallback, &cameraData);

  float positive = 1.0f;
  float negative = -1.0f;
  ammonite::input::registerKeybind(GLFW_KEY_RIGHT_BRACKET, changeFocalDepthCallback, &positive);
  ammonite::input::registerKeybind(GLFW_KEY_LEFT_BRACKET, changeFocalDepthCallback, &negative);



  //Engine loaded, delete the loading screen
  ammonite::utils::status << "Loaded demo in " << utilityTimer.getTime() << "s" << std::endl;
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
    ammonite::utils::controls::processInput();

    //Call demo-specific mainloop code
    if (rendererMainloop != nullptr) {
      if (rendererMainloop() == -1) {
        std::cerr << "ERROR: Failed to run mainloop, exiting" << std::endl;
        return EXIT_FAILURE;
      }
    }
  }

  //Output benchmark score
  if (useBenchmark) {
    std::cout << "\nBenchmark complete:" << std::endl;
    std::cout << "  Average fps: ";
    printMetrics(utilityTimer.getTime() / ammonite::renderer::getTotalFrames());
  }

  //Clean up and exit
  if (demoExit != nullptr) {
    bool cleanExit = true;
    if (demoExit() == -1) {
      cleanExit = false;
      std::cerr << "ERROR: Failed to clean up, exiting" << std::endl;
    }
    ammonite::window::destroyWindow();

    if (!cleanExit) {
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
