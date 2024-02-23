#ifndef RENDERER
#define RENDERER

namespace ammonite {
  namespace renderer {
    namespace setup {
      void setupRenderer(const char* shaderPath, bool* externalSuccess);
      void destroyRenderer();
    }

    long long getTotalFrames();
    double getFrameTime();

    void drawFrame();
  }
}

#endif
