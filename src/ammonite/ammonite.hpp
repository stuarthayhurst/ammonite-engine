#ifndef AMMONITE
#define AMMONITE

#include "graphics/renderInterface.hpp"
#include "graphics/shaders.hpp"

#include "lighting/lightInterface.hpp"
#include "lighting/lightStorage.hpp"

#include "models/modelInterface.hpp"
#include "models/modelStorage.hpp"

#include "utils/cacheManager.hpp"
#include "utils/controls.hpp"
#include "utils/extension.hpp"
#include "utils/keybinds.hpp"
#include "utils/logging.hpp"
#include "utils/timer.hpp"

#ifdef DEBUG
  #include "utils/debug.hpp"
#endif

#include "camera.hpp"
#include "constants.hpp"
#include "environment.hpp"
#include "input.hpp"
#include "interface.hpp"
#include "settings.hpp"
#include "window.hpp"

#endif
