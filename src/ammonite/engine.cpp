#include <string_view>

#include "engine.hpp"

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
}
