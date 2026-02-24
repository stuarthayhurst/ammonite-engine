#ifndef AMMONITEENGINE
#define AMMONITEENGINE

#include <ctime>
#include <string>
#include <string_view>

#include "visibility.hpp"

namespace AMMONITE_EXPOSED ammonite {
  std::string_view getEngineName();
  std::string_view getEngineVersion();

  bool setupEngine(const std::string& shaderPath, unsigned int width,
                   unsigned int height, const std::string& title);
  bool setupEngine(const std::string& shaderPath, unsigned int width,
                   unsigned int height);
  void destroyEngine();

  void pauseEngineTime();
  void unpauseEngineTime();
  bool getEnginePaused();

  void updateFrameTime();
  void getFrameTime(std::time_t* seconds, std::time_t* nanoseconds);
  double getFrameTime();
}

#endif
