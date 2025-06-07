#ifndef AMMONITEENGINE
#define AMMONITEENGINE

#include <string>
#include <string_view>

namespace ammonite {
  std::string_view getEngineName();
  std::string_view getEngineVersion();

  bool setupEngine(const std::string& shaderPath, unsigned int width,
                   unsigned int height, const std::string& title);
  bool setupEngine(const std::string& shaderPath, unsigned int width,
                   unsigned int height);
  void destroyEngine();
}

#endif
