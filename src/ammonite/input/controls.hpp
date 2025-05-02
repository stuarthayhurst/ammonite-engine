#ifndef INTERNALCONTROLS
#define INTERNALCONTROLS

#include "../internal.hpp"

//Include public interface
#include "../../include/ammonite/input/controls.hpp" // IWYU pragma: export

namespace ammonite {
  namespace controls {
    namespace AMMONITE_INTERNAL internal {
      void setCursorFocus(bool inputFocused);
    }
  }
}

#endif
