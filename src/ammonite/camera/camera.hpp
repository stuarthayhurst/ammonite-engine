#ifndef INTERNALCAMERA
#define INTERNALCAMERA

#include "../maths/matrix.hpp"
#include "../utils/id.hpp"
#include "../visibility.hpp"

//Include public interface
#include "../../include/ammonite/camera/camera.hpp" // IWYU pragma: export

namespace ammonite {
  namespace camera {
    namespace AMMONITE_INTERNAL internal {
      ammonite::Mat<float, 4>* getViewMatrixPtr();
      ammonite::Mat<float, 4>* getProjectionMatrixPtr();
      void updateMatrices();
    }

    namespace path {
      namespace AMMONITE_INTERNAL internal {
        bool setLinkedCamera(AmmoniteId pathId, AmmoniteId cameraId);
        void removeCameraLink(AmmoniteId pathId);
        void updateCamera(AmmoniteId pathId);
      }
    }
  }
}

#endif
