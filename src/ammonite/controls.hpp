#ifndef INTERNALCONTROLS
#define INTERNALCONTROLS

#include "internal.hpp"

//Include public interface
#include "../include/ammonite/controls.hpp"

namespace ammonite {
  namespace controls {
    namespace AMMONITE_INTERNAL internal {
      void setCursorFocus(bool inputFocused);
    }
  }
}

#endif
