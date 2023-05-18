#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <map>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "ammonite/ammonite.hpp"
#include "common/argHandler.hpp"

#include "demos/default.hpp"
#include "demos/object-field.hpp"

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
    EXPAND_DEMO("object-field", objectFieldDemo)
  };
}

void printMetrics(double frameTime) {
  double frameRate = 0.0;
  if (frameTime != 0.0) {
    frameRate = 1 / frameTime;
  }

  std::printf("%.2f fps", frameRate);
  std::printf(" (%fms)\n", frameTime * 1000);
}

void cleanUp(int modelCount, std::vector<int> loadedModelIds) {
  //Cleanup
  for (int i = 0; i < modelCount; i++) {
    ammonite::models::deleteModel(loadedModelIds[i]);
  }
  ammonite::windowManager::setup::destroyGlfw();
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

  //Create the window
  auto window = ammonite::windowManager::setupWindow(1024, 768, "OpenGL Experiments");
  if (window == NULL) {
    return EXIT_FAILURE;
  }

  //Set an icon
  ammonite::windowManager::useIconDir("assets/icons/");

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

  //Renderer failed to initialise, clean up and exit
  if (!success) {
    std::cerr << "ERROR: Failed to initialise renderer, exiting" << std::endl;
    ammonite::windowManager::setup::destroyGlfw();
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
      ammonite::windowManager::setup::destroyGlfw();
      return EXIT_FAILURE;
    }
  }

  //Camera ids
  int cameraIds[2] = {0, ammonite::camera::createCamera()};
  int cameraIndex = 0;
  bool cameraToggleHeld = false;

  //Set the camera to the start position
  ammonite::camera::setPosition(0, glm::vec3(0.0f, 0.0f, 5.0f));
  ammonite::camera::setPosition(cameraIds[1], glm::vec3(0.0f, 0.0f, 2.0f));

  //Engine loaded, delete the loading screen
  std::cout << "STATUS: Loaded demo in " << utilityTimer.getTime() << "s" << std::endl;
  ammonite::interface::deleteLoadingScreen(screenId);

  //Create and reset timers for performance metrics
  utilityTimer.reset();
  ammonite::utils::Timer frameTimer;

  //Draw frames until window closed
  while(ammonite::utils::controls::shouldWindowClose()) {
    //Every second, output the framerate
    if (frameTimer.getTime() >= 1.0f) {
      printMetrics(ammonite::renderer::getFrameTime());
      frameTimer.reset();
    }

    //Handle toggling input focus
    static int lastInputToggleState = GLFW_RELEASE;
    int inputToggleState = glfwGetKey(window, GLFW_KEY_C);
    if (lastInputToggleState != inputToggleState) {
      if (lastInputToggleState == GLFW_RELEASE) {
        ammonite::utils::controls::setInputFocus(!ammonite::utils::controls::getInputFocus());
      }

      lastInputToggleState = inputToggleState;
    }

    //Cycle camera when pressed
    if (glfwGetKey(window, GLFW_KEY_B) != GLFW_PRESS) {
      cameraToggleHeld = false;
    } else if (!cameraToggleHeld) {
      cameraToggleHeld = true;
      cameraIndex = (cameraIndex + 1) % (sizeof(cameraIds) / sizeof(cameraIds[0]));
      ammonite::camera::setActiveCamera(cameraIds[cameraIndex]);
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
    ammonite::windowManager::setup::destroyGlfw();

    if (!cleanExit) {
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

/* TODO:
 - Add error handling to demo stages (cleanup + error code + log message)
 - Add cleanup on issues + exit
*/
