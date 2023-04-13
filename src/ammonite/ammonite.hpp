#ifndef AMMONITE
#define AMMONITE

#include "utils/timer.hpp"
#include "utils/logging.hpp"
#include "utils/extension.hpp"
#include "utils/controls.hpp"
#include "utils/keybinds.hpp"
#include "utils/cacheManager.hpp"

#ifdef DEBUG
  #include "utils/debug.hpp"
#endif

#include "constants.hpp"
#include "settings.hpp"

#include "shaders.hpp"
#include "camera.hpp"
#include "renderer.hpp"
#include "lightManager.hpp"
#include "environment.hpp"
#include "interface.hpp"

#include "windowManager.hpp"

#include "models/modelInterface.hpp"
#include "models/modelStorage.hpp"

#endif
