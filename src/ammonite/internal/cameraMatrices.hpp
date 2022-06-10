#ifndef INTERNALCAMERA
#define INTERNALCAMERA

namespace ammonite {
  namespace camera {
    namespace matrices {
      glm::mat4* getViewMatrixPtr();
      glm::mat4* getProjectionMatrixPtr();
    }
  }
}

#endif
