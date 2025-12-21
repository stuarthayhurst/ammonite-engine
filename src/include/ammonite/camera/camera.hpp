#ifndef AMMONITECAMERA
#define AMMONITECAMERA

#include "../maths/vectorTypes.hpp"
#include "../utils/id.hpp"
#include "../visibility.hpp"

namespace AMMONITE_EXPOSED ammonite {
  namespace camera {
    AmmoniteId createCamera();
    void deleteCamera(AmmoniteId cameraId);

    AmmoniteId getActiveCamera();
    void setActiveCamera(AmmoniteId cameraId);

    void getPosition(AmmoniteId cameraId, ammonite::Vec<float, 3>& position);
    void getDirection(AmmoniteId cameraId, ammonite::Vec<float, 3>& direction);
    double getHorizontal(AmmoniteId cameraId);
    double getVertical(AmmoniteId cameraId);
    float getFieldOfView(AmmoniteId cameraId);

    void setPosition(AmmoniteId cameraId, const ammonite::Vec<float, 3>& position);
    void setDirection(AmmoniteId cameraId, const ammonite::Vec<float, 3>& direction);
    void setAngle(AmmoniteId cameraId, double horizontal, double vertical);
    void setFieldOfView(AmmoniteId cameraId, float fov);

    void setLinkedPath(AmmoniteId cameraId, AmmoniteId pathId);
    void removeLinkedPath(AmmoniteId cameraId);

    namespace path {
      AmmoniteId createCameraPath(unsigned int size);
      AmmoniteId createCameraPath();
      void destroyCameraPath(AmmoniteId pathId);
      void reserveCameraPath(AmmoniteId pathId, unsigned int size);

      unsigned int addPathNode(AmmoniteId pathId,
                               const ammonite::Vec<float, 3>& position,
                               double horizontal, double vertical,
                               double time);
      unsigned int addPathNode(AmmoniteId pathId,
                               const ammonite::Vec<float, 3>& position,
                               const ammonite::Vec<float, 3>& direction,
                               double time);
      void removePathNode(AmmoniteId pathId, unsigned int nodeIndex);
      unsigned int getPathNodeCount(AmmoniteId pathId);

      void playPath(AmmoniteId pathId);
      void pausePath(AmmoniteId pathId);
      void restartPath(AmmoniteId pathId);
    }
  }
}

#endif
