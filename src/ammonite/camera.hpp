#ifndef INTERNALCAMERA
#define INTERNALCAMERA

#include "internal.hpp"

#include "maths/matrix.hpp"

//Include public interface
#include "../include/ammonite/camera.hpp" // IWYU pragma: export

namespace ammonite {
  namespace camera {
    namespace AMMONITE_INTERNAL internal {
      ammonite::Mat<float, 4>* getViewMatrixPtr();
      ammonite::Mat<float, 4>* getProjectionMatrixPtr();
      void updateMatrices();
    }
  }
}

#endif
