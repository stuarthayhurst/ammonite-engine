#ifndef INTERNALSETTINGS
#define INTERNALSETTINGS

namespace ammonite {
  namespace settings {
    namespace runtime {
      float* getAspectRatioPtr();

      int getWidth();
      int getHeight();

      void setWidth(int width);
      void setHeight(int height);
    }
  }
}

#endif
