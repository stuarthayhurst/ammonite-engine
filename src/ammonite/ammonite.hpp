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

#include "camera.hpp"
#include "environment.hpp"
#include "interface.hpp"

#include "window.hpp"

#include "graphics/shaders.hpp"
#include "graphics/renderInterface.hpp"

#include "models/modelInterface.hpp"
#include "models/modelStorage.hpp"

#include "lighting/lightInterface.hpp"
#include "lighting/lightStorage.hpp"

#endif
