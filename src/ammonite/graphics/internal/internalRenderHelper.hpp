/* Internally exposed header:
 - Expose rendering helpers to render core
*/

#ifndef INTERNALRENDERERHELPER
#define INTERNALRENDERERHELPER

namespace ammonite {
  namespace renderer {
    namespace internal {
      void prepareScreen(int framebufferId, int width, int height, bool depthTest);
    }
  }
}

#endif
