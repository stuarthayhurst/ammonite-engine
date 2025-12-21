#include <cmath>
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
        double horizontalAngle = ammonite::pi<double>();
        double verticalAngle = 0.0f;
        float fov = ammonite::pi<float>() / 4.0f;
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

    namespace {
      void calculateDirection(float horizontal, float vertical,
                              ammonite::Vec<float, 3>& dest) {
        const ammonite::Vec<float, 3> direction = {
          std::cos(vertical) * std::sin(horizontal),
          std::sin(vertical),
          std::cos(vertical) * std::cos(horizontal)
        };

        ammonite::normalise(direction, dest);
      }

      double calculateVerticalAngle(const ammonite::Vec<float, 3>& direction) {
        ammonite::Vec<float, 3> normalisedDirection = {0};
        ammonite::normalise(direction, normalisedDirection);
        return std::asin(normalisedDirection[1]);
      }

      double calculateHorizontalAngle(const ammonite::Vec<float, 3>& direction) {
        ammonite::Vec<float, 3> normalisedDirection = {0};
        ammonite::copy(direction, normalisedDirection);
        normalisedDirection[1] = 0.0f;

        ammonite::normalise(normalisedDirection);
        return std::atan2(normalisedDirection[0], normalisedDirection[2]);
      }
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

        //If the camera is on a path, update it
        if (activeCamera->linkedCameraPathId != 0) {
          ammonite::camera::path::internal::updateCamera(activeCamera->linkedCameraPathId);
        }

        //Calculate the direction vector
        ammonite::Vec<float, 3> direction = {0};
        calculateDirection((float)activeCamera->horizontalAngle,
                           (float)activeCamera->verticalAngle, direction);

        //Right vector, relative to the camera
        const ammonite::Vec<float, 3> right = {
          std::sin((float)activeCamera->horizontalAngle - (ammonite::pi<float>() / 2.0f)),
          0.0f,
          std::cos((float)activeCamera->horizontalAngle - (ammonite::pi<float>() / 2.0f))
        };

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

      void forceRemoveLinkedPath(AmmoniteId cameraId) {
        if (cameraTrackerMap.contains(cameraId)) {
          cameraTrackerMap[cameraId].linkedCameraPathId = 0;
        }
      }
    }

    AmmoniteId getActiveCamera() {
      return activeCameraId;
    }

    void setActiveCamera(AmmoniteId cameraId) {
      //If the camera exists, set as active
      if (cameraTrackerMap.contains(cameraId)) {
        activeCameraId = cameraId;
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
        return;
      }

      //Reset any camera path link
      const AmmoniteId linkedPathId = cameraTrackerMap[cameraId].linkedCameraPathId;
      if (linkedPathId != 0) {
        path::internal::removeLinkedCamera(linkedPathId);
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
      if (cameraTrackerMap.contains(cameraId)) {
        ammonite::copy(cameraTrackerMap[cameraId].position, position);
        return;
      }

      ammonite::set(position, 0.0f);
    }

    void getDirection(AmmoniteId cameraId, ammonite::Vec<float, 3>& direction) {
      if (cameraTrackerMap.contains(cameraId)) {
        const Camera& activeCamera = cameraTrackerMap[cameraId];
        calculateDirection((float)activeCamera.horizontalAngle,
                           (float)activeCamera.verticalAngle, direction);
        return;
      }

      ammonite::set(direction, 0.0f);
    }

    //Get horizontal angle (radians)
    double getHorizontal(AmmoniteId cameraId) {
      if (cameraTrackerMap.contains(cameraId)) {
        return cameraTrackerMap[cameraId].horizontalAngle;
      }

      return 0.0;
    }

    //Get vertical angle (radians)
    double getVertical(AmmoniteId cameraId) {
      if (cameraTrackerMap.contains(cameraId)) {
        return cameraTrackerMap[cameraId].verticalAngle;
      }

      return 0.0;
    }

    //Get field of view (radians)
    float getFieldOfView(AmmoniteId cameraId) {
      if (cameraTrackerMap.contains(cameraId)) {
        return cameraTrackerMap[cameraId].fov;
      }

      return ammonite::pi<float>() / 4.0f;
    }

    //Set position
    void setPosition(AmmoniteId cameraId, const ammonite::Vec<float, 3>& position) {
      //Find the target camera and update position
      if (cameraTrackerMap.contains(cameraId)) {
        ammonite::copy(position, cameraTrackerMap[cameraId].position);
      }
    }

    //Set direction
    void setDirection(AmmoniteId cameraId, const ammonite::Vec<float, 3>& direction) {
      //Find the target camera and update direction
      if (cameraTrackerMap.contains(cameraId)) {
        Camera& activeCamera = cameraTrackerMap[cameraId];
        activeCamera.horizontalAngle = calculateHorizontalAngle(direction);
        activeCamera.verticalAngle = calculateVerticalAngle(direction);
      }
    }

    //Set camera direction via angle pair (radians)
    void setAngle(AmmoniteId cameraId, double horizontal, double vertical) {
      //Convert to a vector and set it
      if (cameraTrackerMap.contains(cameraId)) {
        cameraTrackerMap[cameraId].horizontalAngle = horizontal;
        cameraTrackerMap[cameraId].verticalAngle = vertical;
      }
    }

    //Set field of view (radians)
    void setFieldOfView(AmmoniteId cameraId, float fov) {
      //Find the target camera and update field of view
      if (cameraTrackerMap.contains(cameraId)) {
        cameraTrackerMap[cameraId].fov = fov;
      }
    }

    void setLinkedPath(AmmoniteId cameraId, AmmoniteId pathId) {
      //Store the ID of the linked path
      if (cameraTrackerMap.contains(cameraId)) {
        //Remove the old link
        path::internal::removeLinkedCamera(cameraTrackerMap[cameraId].linkedCameraPathId);

        //Record the new link in this system
        cameraTrackerMap[cameraId].linkedCameraPathId = pathId;

        //Set new link in the path system
        if (pathId != 0 && !path::internal::setLinkedCamera(pathId, cameraId)) {
          ammonite::utils::warning << "Failed to link camera (ID " << cameraId \
                                   << ") and path (ID " << pathId \
                                   << "), resetting link" << std::endl;
          cameraTrackerMap[cameraId].linkedCameraPathId = 0;
        }
      }
    }

    void removeLinkedPath(AmmoniteId cameraId) {
      setLinkedPath(cameraId, 0);
    }
  }
}
