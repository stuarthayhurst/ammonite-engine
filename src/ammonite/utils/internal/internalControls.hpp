#ifndef INTERNALCONTROLS
#define INTERNALCONTROLS

/* Internally exposed header:
 - Expose function to set input focus in controls
*/

namespace ammonite {
  namespace utils {
    namespace controls {
      namespace internal {
        void setCursorFocus(bool inputFocused);
      }
    }
  }
}

#endif
