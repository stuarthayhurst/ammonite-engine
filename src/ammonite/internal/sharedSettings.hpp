#ifndef INTERNALSETTINGS
#define INTERNALSETTINGS

namespace ammonite {
  namespace settings {
    int getWidth();
    int getHeight();
    float* getAspectRatioPtr();

    void setWidth(int width);
    void setHeight(int height);
  }
}

#endif
