#ifndef COMMANDS
#define COMMANDS

#include <ammonite/ammonite.hpp>

namespace commands {
  bool commandPrompt();

  void registerCameraPath(AmmoniteId pathId);
  void deleteCameraPaths();
}

#endif
