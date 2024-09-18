#ifndef INTERNALCONTROLS
#define INTERNALCONTROLS

/* Internally exposed header:
 - Expose function to set cursor focus from input system
*/

namespace ammonite {
  namespace controls {
    namespace internal {
      void setCursorFocus(bool inputFocused);
    }
  }
}

#endif
