#ifndef INTERNALSHADERS
#define INTERNALSHADERS

/* Internally exposed header:
 - Allow window manager to prompt checking for cache support
*/

namespace ammonite {
  namespace shaders {
    namespace internal {
      void updateGLCacheSupport();
    }
  }
}

#endif
