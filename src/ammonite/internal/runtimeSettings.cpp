namespace ammonite {
  namespace settings {
    namespace runtime {
      namespace {
        unsigned int width = 0, height = 0;
        float aspectRatio = 0.0f;
      }

      float* getAspectRatioPtr() {
        return &aspectRatio;
      }

      int getWidth() {
        return width;
      }

      int getHeight() {
        return height;
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
}
