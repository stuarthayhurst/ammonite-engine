#ifndef AMMONITEENGINE
#define AMMONITEENGINE

#include <string>
#include <string_view>

#include "exposed.hpp"

namespace AMMONITE_EXPOSED ammonite {
  std::string_view getEngineName();
  std::string_view getEngineVersion();

  bool setupEngine(const std::string& shaderPath, unsigned int width,
                   unsigned int height, const std::string& title);
  bool setupEngine(const std::string& shaderPath, unsigned int width,
                   unsigned int height);
  void destroyEngine();
}

#endif
