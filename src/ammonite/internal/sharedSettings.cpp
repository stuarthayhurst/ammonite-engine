namespace ammonite {
  namespace settings {
    namespace {
      unsigned int width = 0, height = 0;
      float aspectRatio = 0.0f;
    }

    int getWidth() {
      return width;
    }

    int getHeight() {
      return height;
    }

    float* getAspectRatioPtr() {
      return &aspectRatio;
    }

    void setWidth(int newWidth) {
      width = newWidth;
      aspectRatio = float(width) / float(height);
    }

    void setHeight(int newHeight) {
      height = newHeight;
      aspectRatio = float(width) / float(height);
    }
  }
}
