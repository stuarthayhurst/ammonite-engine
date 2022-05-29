#include <iostream>
#include <cstdlib>
#include <tuple>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "ammonite/ammonite.hpp"
#include "common/argHandler.hpp"

//Initial width and height
const unsigned short int width = 1024;
const unsigned short int height = 768;

void printMetrics(double frameTime) {
  std::printf("%.2f fps", 1 / frameTime);
  std::printf(" (%fms)\n", frameTime * 1000);
}

void cleanUp(int programId, int modelCount, int loadedModelIds[]) {
  //Cleanup
  for (int i = 0; i < modelCount; i++) {
    ammonite::models::deleteModel(loadedModelIds[i]);
  }
  ammonite::shaders::eraseShaders();
  glDeleteProgram(programId);
  ammonite::windowManager::setup::destroyGlfw();
}

int main(int argc, char* argv[]) {
  //Handle arguments
  const int showHelp = arguments::searchArgument(argc, argv, "--help", true, nullptr);
  if (showHelp == 1) {
    std::cout << "Program help: \n"
    " --help:       Display this help page\n"
    " --benchmark:  Start a benchmark\n"
    " --vsync:      Enable / disable VSync (true / false)" << std::endl;
    return EXIT_SUCCESS;
  } else if (showHelp == -1) {
    return EXIT_FAILURE;
  }

  const bool useBenchmark = arguments::searchArgument(argc, argv, "--benchmark", true, nullptr);

  std::string useVsync;
  if (arguments::searchArgument(argc, argv, "--vsync", false, &useVsync) == -1) {
    std::cout << "--vsync requires a value" << std::endl;
    return EXIT_FAILURE;
  }

  //Create the window
  auto [window, widthPtr, heightPtr, aspectRatioPtr] = ammonite::windowManager::setupWindow(width, height, 4, "OpenGL Experiments");
  if (window == NULL) {
    return EXIT_FAILURE;
  }

  //Set an icon
  ammonite::windowManager::useIconDir(window, "assets/icons/");

  //Initialise controls
  ammonite::controls::setupControls(window, widthPtr, heightPtr, aspectRatioPtr);

  //Set vsync (disable if benchmarking)
  if (useVsync == "false" or useBenchmark == true) {
    ammonite::windowManager::settings::useVsync(false);
  } else if (useVsync == "true") {
    ammonite::windowManager::settings::useVsync(true);
  }

#ifdef DEBUG
  ammonite::utils::debug::enableDebug();
#endif

  //Enable binary caching
  ammonite::shaders::useProgramCache("cache");

  //Create program from shaders
  bool success = true;
  ammonite::utils::Timer performanceTimer;
  GLuint programId = ammonite::shaders::loadDirectory("shaders/models/", &success);
  GLuint lightShaderId = ammonite::shaders::loadDirectory("shaders/lights/", &success);

  if (!success) {
    std::cerr << "Program creation failed" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Loaded shaders in: " << performanceTimer.getTime() << "s" << std::endl;

  //Load models from a set of objects and textures
  const char* models[][2] = {
    {"assets/suzanne.obj", "assets/gradient.png"},
    {"assets/cube.obj", "assets/texture.bmp"}
  };
  int modelCount = sizeof(models) / sizeof(models[0]);
  int loadedModelIds[modelCount];

  long int vertexCount = 0;
  performanceTimer.reset();
  for (int i = 0; i < modelCount; i++) {
    //Load model
    loadedModelIds[i] = ammonite::models::createModel(models[i][0], &success);

    //Count vertices
    vertexCount += ammonite::models::getVertexCount(loadedModelIds[i]);

    //Load texture
    ammonite::models::applyTexture(loadedModelIds[i], models[i][1], &success);
  }

  //Example translation, scale and rotation
  ammonite::models::position::translateModel(loadedModelIds[0], glm::vec3(-1.0f, 0.0f, 0.0f));
  ammonite::models::position::scaleModel(loadedModelIds[0], 0.8f);
  ammonite::models::position::rotateModel(loadedModelIds[0], glm::vec3(0.0f, 0.0f, 0.0f));

  //Destroy all models, textures and shaders then exit
  if (!success) {
    cleanUp(programId, modelCount, loadedModelIds);
    return EXIT_FAILURE;
  }

  std::cout << "Loaded models in: " << performanceTimer.getTime() << "s (" << vertexCount << " vertices)" << std::endl;

  //Create light sources
  ammonite::lighting::setAmbientLight(glm::vec3(0.1f, 0.1f, 0.1f));
  int lightId = ammonite::lighting::createLightSource();

  //Set light source properties
  ammonite::lighting::LightSource* lightSource = ammonite::lighting::getLightSourcePtr(lightId);
  lightSource->geometry = glm::vec3(4.0f, 4.0f, 4.0f);
  lightSource->power = 50.0f;
  ammonite::lighting::updateLightSources();

  //Setup the renderer
  glm::mat4 projectionMatrix, viewMatrix;
  ammonite::renderer::setup::setupRenderer(window, programId, lightShaderId, &success);
  ammonite::renderer::setup::setupMatrices(&projectionMatrix, &viewMatrix);

  //Renderer failed to initialise, clean up and exit
  if (!success) {
    std::cerr << "Failed to initialise renderer, exiting" << std::endl;
    cleanUp(programId, modelCount, loadedModelIds);
    return EXIT_FAILURE;
  }

  //Performance metrics setup
  ammonite::utils::Timer benchmarkTimer;
  performanceTimer.reset();

  //Draw frames until window closed
  while(ammonite::controls::shouldWindowClose()) {
    //Every second, output the framerate
    if (performanceTimer.getTime() >= 1.0f) {
      printMetrics(ammonite::renderer::getFrameTime());
      performanceTimer.reset();
    }

    //Process new input since last frame
    ammonite::controls::processInput();

    //Get view and projection matrices
    projectionMatrix = ammonite::controls::matrix::getProjectionMatrix();
    viewMatrix = ammonite::controls::matrix::getViewMatrix();

    ammonite::renderer::drawFrame(loadedModelIds, modelCount);
  }

  //Output benchmark score
  if (useBenchmark) {
    std::cout << "\nBenchmark complete:" << std::endl;
    std::cout << "  Average fps: ";
    printMetrics(benchmarkTimer.getTime() / ammonite::renderer::getTotalFrames());
  }

  //Clean up and exit
  cleanUp(programId, modelCount, loadedModelIds);
  return EXIT_SUCCESS;
}
