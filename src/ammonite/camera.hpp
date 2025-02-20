#ifndef INTERNALCAMERA
#define INTERNALCAMERA

#include <glm/glm.hpp>

#include "internal.hpp"

//Include public interface
#include "../include/ammonite/camera.hpp" // IWYU pragma: export

namespace ammonite {
  namespace camera {
    namespace AMMONITE_INTERNAL internal {
      glm::mat4* getViewMatrixPtr();
      glm::mat4* getProjectionMatrixPtr();
      void updateMatrices();
    }
  }
}

#endif
