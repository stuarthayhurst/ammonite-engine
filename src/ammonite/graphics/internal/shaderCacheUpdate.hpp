#ifndef INTERNALSHADERCACHEUPDATE
#define INTERNALSHADERCACHEUPDATE

/* Internally exposed header:
 - Allow window manager to prompt checking for cache support
*/

namespace ammonite {
  namespace shaders {
    void updateGLCacheSupport();
  }
}

#endif
