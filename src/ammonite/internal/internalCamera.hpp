#ifndef INTERNALCAMERA
#define INTERNALCAMERA

/* Internally exposed header:
 - Allow renderer to request pointers to the view and projection matrices
 - Allow settings to recalculate matrices
*/

#include <glm/glm.hpp>

namespace ammonite {
  namespace camera {
    namespace internal {
      glm::mat4* getViewMatrixPtr();
      glm::mat4* getProjectionMatrixPtr();
      void updateMatrices();
    }
  }
}

#endif
