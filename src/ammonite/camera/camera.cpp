#include <iostream>
#include <unordered_map>

#include "camera.hpp"

#include "../graphics/renderer.hpp"
#include "../maths/angle.hpp"
#include "../maths/matrix.hpp"
#include "../maths/vector.hpp"
#include "../utils/debug.hpp"
#include "../utils/id.hpp"
#include "../utils/logging.hpp"
#include "../window/window.hpp"

namespace ammonite {
  namespace camera {
    namespace {
      struct Camera {
        ammonite::Vec<float, 3> position = {0};
        double horizontalAngle = ammonite::pi<double>;
        double verticalAngle = 0.0f;
        float fov = ammonite::pi<float> / 4.0f;
        AmmoniteId linkedCameraPathId = 0;
      } defaultCamera;

      //View and projection matrices
      ammonite::Mat<float, 4> viewMatrix = {{0}};
      ammonite::Mat<float, 4> projectionMatrix = {{0}};

      //Create map to track cameras, with default camera
      AmmoniteId lastCameraId = 1;
      AmmoniteId activeCameraId = 1;
      std::unordered_map<AmmoniteId, Camera> cameraTrackerMap = {{1, defaultCamera}};
    }

    //Pointer and update methods exposed internally
    namespace internal {
      ammonite::Mat<float, 4>* getViewMatrixPtr() {
        return &viewMatrix;
      }

      ammonite::Mat<float, 4>* getProjectionMatrixPtr() {
        return &projectionMatrix;
      }

      void updateMatrices() {
        //Get the active camera
        Camera* const activeCamera = &cameraTrackerMap[activeCameraId];

        /*
         - If the camera is on a path, update it if the user hasn't
         - If the camera isn't on a path, clear the cached set of updated cameras
        */
        camera::path::internal::ensureCameraUpdatedForPath(activeCamera->linkedCameraPathId);

        //Calculate the direction vector
        ammonite::Vec<float, 3> direction = {0};
        ammonite::calculateDirection((float)activeCamera->horizontalAngle,
                                     (float)activeCamera->verticalAngle, direction);

        //Right vector, relative to the camera
        ammonite::Vec<float, 3> right = {0};
        const float angleRight = (float)activeCamera->horizontalAngle - ammonite::halfPi<float>;
        ammonite::calculateDirection(angleRight, right);

        //Up vector, relative to the camera
        ammonite::Vec<float, 3> up = {0};
        ammonite::cross(right, direction, up);

        //Calculate the projection matrix from FoV, aspect ratio and display range
        const float aspectRatio = ammonite::window::internal::getGraphicsAspectRatio();
        const float renderFarPlane = ammonite::renderer::settings::getRenderFarPlane();
        ammonite::perspective(activeCamera->fov, aspectRatio, 0.1f,
                              renderFarPlane, projectionMatrix);

        //Calculate view matrix from position, where it's looking, and relative up
        ammonite::Vec<float, 3> cameraTarget = {0};
        ammonite::add(activeCamera->position, direction, cameraTarget);
        ammonite::lookAt(activeCamera->position, cameraTarget, up, viewMatrix);
      }

      //Update the stored link for cameraId, optionally unlink the existing path
      bool setLinkedPath(AmmoniteId cameraId, AmmoniteId pathId,
                         bool unlinkExisting) {
        //Ignore reset requests for camera 0
        if (pathId == 0 && cameraId == 0) {
          ammoniteInternalDebug << "Ignored path reset request for camera ID 0" << std::endl;
          return true;
        }

        //Check the camera exists
        if (!cameraTrackerMap.contains(cameraId)) {
          ammonite::utils::warning << "Can't find camera (ID " \
                                   << cameraId << ") to unlink" << std::endl;
          return false;
        }

        //Reset the linked camera on any already linked path, if requested
        if (unlinkExisting) {
          if (!path::internal::setLinkedCamera(cameraTrackerMap[cameraId].linkedCameraPathId, 0, false)) {
            ammonite::utils::warning << "Failed to unlink path (ID " \
                                     << cameraTrackerMap[cameraId].linkedCameraPathId \
                                     << ") from camera (ID " << cameraId << ")" << std::endl;
            return false;
          }
        }

        //Set the path on the camera
        cameraTrackerMap[cameraId].linkedCameraPathId = pathId;
        return true;
      }
    }

    AmmoniteId getActiveCamera() {
      return activeCameraId;
    }

    void setActiveCamera(AmmoniteId cameraId) {
      //If the camera exists, set as active
      if (cameraTrackerMap.contains(cameraId)) {
        activeCameraId = cameraId;
      } else {
        ammonite::utils::warning << "Couldn't find camera with ID '" << cameraId \
                                 << "'" << std::endl;
      }
    }

    AmmoniteId createCamera() {
      //Get an ID for the new camera
      const AmmoniteId cameraId = utils::internal::setNextId(&lastCameraId, cameraTrackerMap);

      //Add a new camera to the tracker
      cameraTrackerMap[cameraId] = {};

      return cameraId;
    }

    void deleteCamera(AmmoniteId cameraId) {
      //Check the camera exists
      if (!cameraTrackerMap.contains(cameraId)) {
        ammonite::utils::warning << "Couldn't find camera with ID '" << cameraId \
                                 << "'" << std::endl;
        return;
      }

      //Reset any camera path link
      const AmmoniteId linkedPathId = cameraTrackerMap[cameraId].linkedCameraPathId;
      if (!path::internal::setLinkedCamera(linkedPathId, 0, false)) {
        ammonite::utils::warning << "Failed to unlink camera path (ID " \
                                 << linkedPathId << ")" << std::endl;
      }

      //Delete the camera, if it's not the default
      if (cameraId != 1) {
        cameraTrackerMap.erase(cameraId);
        ammoniteInternalDebug << "Deleted storage for camera (ID " \
                              << cameraId << ")" << std::endl;
      }

      //If the deleted camera was the active camera, reset
      if (activeCameraId == cameraId) {
        setActiveCamera(1);
      }
    }

    //Get position
    void getPosition(AmmoniteId cameraId, ammonite::Vec<float, 3>& position) {
      if (!cameraTrackerMap.contains(cameraId)) {
        ammonite::utils::warning << "Couldn't find camera with ID '" << cameraId \
                                 << "'" << std::endl;
        ammonite::set(position, 0.0f);
        return;
      }

      ammonite::copy(cameraTrackerMap[cameraId].position, position);
    }

    void getDirection(AmmoniteId cameraId, ammonite::Vec<float, 3>& direction) {
      if (!cameraTrackerMap.contains(cameraId)) {
        ammonite::utils::warning << "Couldn't find camera with ID '" << cameraId \
                                 << "'" << std::endl;
        ammonite::set(direction, 0.0f);
        return;
      }

      const Camera& activeCamera = cameraTrackerMap[cameraId];
      ammonite::calculateDirection((float)activeCamera.horizontalAngle,
                                   (float)activeCamera.verticalAngle, direction);
    }

    //Get horizontal angle (radians)
    double getHorizontal(AmmoniteId cameraId) {
      if (!cameraTrackerMap.contains(cameraId)) {
        ammonite::utils::warning << "Couldn't find camera with ID '" << cameraId \
                                 << "'" << std::endl;
        return 0.0;
      }

      return cameraTrackerMap[cameraId].horizontalAngle;

    }

    //Get vertical angle (radians)
    double getVertical(AmmoniteId cameraId) {
      if (!cameraTrackerMap.contains(cameraId)) {
        ammonite::utils::warning << "Couldn't find camera with ID '" << cameraId \
                                 << "'" << std::endl;
        return 0.0;
      }

      return cameraTrackerMap[cameraId].verticalAngle;
    }

    //Get field of view (radians)
    float getFieldOfView(AmmoniteId cameraId) {
      if (!cameraTrackerMap.contains(cameraId)) {
        ammonite::utils::warning << "Couldn't find camera with ID '" << cameraId \
                                 << "'" << std::endl;
        return ammonite::pi<float> / 4.0f;
      }

      return cameraTrackerMap[cameraId].fov;
    }

    //Set position
    void setPosition(AmmoniteId cameraId, const ammonite::Vec<float, 3>& position) {
      //Check the camera exists
      if (!cameraTrackerMap.contains(cameraId)) {
        ammonite::utils::warning << "Couldn't find camera with ID '" << cameraId \
                                 << "'" << std::endl;
        return;
      }

      //Find the target camera and update position
      ammonite::copy(position, cameraTrackerMap[cameraId].position);
    }

    //Set direction
    void setDirection(AmmoniteId cameraId, const ammonite::Vec<float, 3>& direction) {
      //Check the camera exists
      if (!cameraTrackerMap.contains(cameraId)) {
        ammonite::utils::warning << "Couldn't find camera with ID '" << cameraId \
                                 << "'" << std::endl;
        return;
      }

      //Find the target camera and update direction
      Camera& activeCamera = cameraTrackerMap[cameraId];
      activeCamera.horizontalAngle = ammonite::calculateHorizontalAngle(direction);
      activeCamera.verticalAngle = ammonite::calculateVerticalAngle(direction);
    }

    //Set camera direction via angle pair (radians)
    void setAngle(AmmoniteId cameraId, double horizontal, double vertical) {
      //Check the camera exists
      if (!cameraTrackerMap.contains(cameraId)) {
        ammonite::utils::warning << "Couldn't find camera with ID '" << cameraId \
                                 << "'" << std::endl;
        return;
      }

      //Convert to a vector and set it
      cameraTrackerMap[cameraId].horizontalAngle = horizontal;
      cameraTrackerMap[cameraId].verticalAngle = vertical;
    }

    //Set field of view (radians)
    void setFieldOfView(AmmoniteId cameraId, float fov) {
      //Check the camera exists
      if (!cameraTrackerMap.contains(cameraId)) {
        ammonite::utils::warning << "Couldn't find camera with ID '" << cameraId \
                                 << "'" << std::endl;
        return;
      }

      //Find the target camera and update field of view
      cameraTrackerMap[cameraId].fov = fov;
    }

    /*
     - Unlink any existing path from cameraId
     - Unlink any existing camera from pathId
     - Create a new link between cameraId and pathId
    */
    void setLinkedPath(AmmoniteId cameraId, AmmoniteId pathId) {
      //Check the camera exists
      if (!cameraTrackerMap.contains(cameraId)) {
        ammonite::utils::warning << "Couldn't find camera with ID '" << cameraId \
                                 << "'" << std::endl;
        return;
      }

      /*
       - Unlink the old camera from the new path
       - Unlink the old path from the new camera
       - Record the new link in both systems
      */
      const bool success = internal::setLinkedPath(cameraId, pathId, true) &&
                           path::internal::setLinkedCamera(pathId, cameraId, true);
      if (!success) {
        ammonite::utils::warning << "Failed to link camera (ID " << cameraId \
                                 << ") and path (ID " << pathId \
                                 << ")" << std::endl;
        return;
      }
    }

    AmmoniteId getLinkedPath(AmmoniteId cameraId) {
      //Check the camera exists
      if (!cameraTrackerMap.contains(cameraId)) {
        ammonite::utils::warning << "Couldn't find camera with ID '" << cameraId \
                                 << "'" << std::endl;
        return 0;
      }

      return cameraTrackerMap[cameraId].linkedCameraPathId;
    }

    bool isCameraLinked(AmmoniteId cameraId) {
      return (getLinkedPath(cameraId) != 0);
    }

    void removeLinkedPath(AmmoniteId cameraId) {
      //Check the camera exists
      if (!cameraTrackerMap.contains(cameraId)) {
        ammonite::utils::warning << "Couldn't find camera with ID '" << cameraId \
                                 << "'" << std::endl;
        return;
      }

      //Instruct the path system to unlink
      const AmmoniteId pathId = cameraTrackerMap[cameraId].linkedCameraPathId;
      if (!path::internal::setLinkedCamera(pathId, 0, true)) {
        ammonite::utils::warning << "Failed to unlink camera (ID " << cameraId \
                                 << ") and path (ID " << pathId \
                                 << ")" << std::endl;
      }
    }
  }
}
