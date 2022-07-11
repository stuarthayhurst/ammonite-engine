#ifndef INTERNALCAMERA
#define INTERNALCAMERA

/* Internally exposed header:
 - Allow renderer to request pointers to the view and projection matrices
*/

namespace ammonite {
  namespace camera {
    namespace matrices {
      glm::mat4* getViewMatrixPtr();
      glm::mat4* getProjectionMatrixPtr();
    }
  }
}

#endif
