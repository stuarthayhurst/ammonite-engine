#ifndef SETTINGS
#define SETTINGS

namespace ammonite {
  namespace settings {
    namespace graphics {
      void setVsync(bool enabled);
      bool getVsync();
      void setFrameLimit(float frameTarget);
      float getFrameLimit();
      void setShadowRes(int shadowRes);
      int getShadowRes();
    }
  }
}

#endif