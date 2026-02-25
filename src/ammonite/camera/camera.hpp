#ifndef INTERNALCAMERA
#define INTERNALCAMERA

#include "../maths/matrix.hpp"
#include "../utils/id.hpp"
#include "../visibility.hpp"

//Include public interface
#include "../../include/ammonite/camera/camera.hpp" // IWYU pragma: export

namespace AMMONITE_INTERNAL ammonite {
  namespace camera {
    namespace internal {
      ammonite::Mat<float, 4>* getViewMatrixPtr();
      ammonite::Mat<float, 4>* getProjectionMatrixPtr();
      void updateMatrices();

      bool setLinkedPath(AmmoniteId cameraId, AmmoniteId pathId,
                         bool unlinkExisting);
    }

    namespace path {
      namespace internal {
        bool setLinkedCamera(AmmoniteId pathId, AmmoniteId cameraId,
                             bool unlinkExisting);

        void ensureCameraUpdatedForPath(AmmoniteId pathId);
      }
    }
  }
}

#endif
