#include <cmath>
#include <map>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include "camera.hpp"

#include "graphics/renderer.hpp"
#include "types.hpp"
#include "window/window.hpp"

namespace ammonite {
  namespace camera {
    namespace {
      struct Camera {
        glm::vec3 position = glm::vec3(0.0f);
        float horizontalAngle = glm::pi<float>();
        float verticalAngle = 0.0f;
        float fov = glm::quarter_pi<float>();
      } defaultCamera;

      //View and projection matrices
      glm::mat4 viewMatrix;
      glm::mat4 projectionMatrix;
      float* renderFarPlanePtr = ammonite::renderer::settings::internal::getRenderFarPlanePtr();

      //Create map to track cameras, with default camera
      unsigned int totalUserCameras = 0;
      AmmoniteId activeCameraId = 1;
      std::map<AmmoniteId, Camera> cameraTrackerMap = {{1, defaultCamera}};
    }

    namespace {
      glm::vec3 calculateDirection(double vertical, double horizontal) {
        return glm::vec3(
          std::cos(vertical) * std::sin(horizontal),
          std::sin(vertical),
          std::cos(vertical) * std::cos(horizontal)
        );
      }
    }

    //Pointer and update methods exposed internally
    namespace internal {
      glm::mat4* getViewMatrixPtr() {
        return &viewMatrix;
      }

      glm::mat4* getProjectionMatrixPtr() {
        return &projectionMatrix;
      }

      void updateMatrices() {
        //Get the active camera
        Camera* activeCamera = &cameraTrackerMap[activeCameraId];

        //Vector for current direction faced
        glm::vec3 direction = calculateDirection(activeCamera->verticalAngle,
                                                 activeCamera->horizontalAngle);

        //Right vector, relative to the camera
        glm::vec3 right = glm::vec3(
          std::sin(activeCamera->horizontalAngle - glm::half_pi<float>()),
          0,
          std::cos(activeCamera->horizontalAngle - glm::half_pi<float>())
        );
        //Up vector, relative to the camera
        glm::vec3 up = glm::cross(right, direction);

        //Calculate the projection matrix from FoV, aspect ratio and display range
        float aspectRatio = ammonite::window::internal::getGraphicsAspectRatio();
        projectionMatrix = glm::perspective(activeCamera->fov,
                                            aspectRatio, 0.1f, *renderFarPlanePtr);

        //Calculate view matrix from position, where it's looking, and relative up
        viewMatrix = glm::lookAt(activeCamera->position, activeCamera->position + direction, up);
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
      totalUserCameras++;
      AmmoniteId cameraId = 1 + totalUserCameras;

      //Add the new camera to the tracker
      Camera newCamera;
      cameraTrackerMap[cameraId] = newCamera;

      return cameraId;
    }

    void deleteCamera(AmmoniteId cameraId) {
      //Delete the camera if present
      if (cameraTrackerMap.contains(cameraId) && cameraId != 1) {
        cameraTrackerMap.erase(cameraId);
      }

      //If the deleted camera was the active camera, reset
      if (activeCameraId == cameraId) {
        setActiveCamera(1);
      }
    }

    //Get position
    glm::vec3 getPosition(AmmoniteId cameraId) {
      if (cameraTrackerMap.contains(cameraId)) {
        return cameraTrackerMap[cameraId].position;
      }

      return glm::vec3(0.0f);
    }

    glm::vec3 getDirection(AmmoniteId cameraId) {
      if (cameraTrackerMap.contains(cameraId)) {
        Camera* activeCamera = &cameraTrackerMap[cameraId];
        glm::vec3 direction = calculateDirection(activeCamera->verticalAngle,
                                                 activeCamera->horizontalAngle);
        return glm::normalize(direction);
      }

      return glm::vec3(0.0f);
    }

    //Get horizontal angle (radians)
    float getHorizontal(AmmoniteId cameraId) {
      if (cameraTrackerMap.contains(cameraId)) {
        return cameraTrackerMap[cameraId].horizontalAngle;
      }

      return 0.0f;
    }

    //Get vertical angle (radians)
    float getVertical(AmmoniteId cameraId) {
      if (cameraTrackerMap.contains(cameraId)) {
        return cameraTrackerMap[cameraId].verticalAngle;
      }

      return 0.0f;
    }

    //Get field of view (radians)
    float getFieldOfView(AmmoniteId cameraId) {
      if (cameraTrackerMap.contains(cameraId)) {
        return cameraTrackerMap[cameraId].fov;
      }

      return glm::quarter_pi<float>();
    }

    //Set position
    void setPosition(AmmoniteId cameraId, glm::vec3 newPosition) {
      //Find the target camera and update position
      if (cameraTrackerMap.contains(cameraId)) {
        cameraTrackerMap[cameraId].position = newPosition;
      }
    }

    //Set horizontal angle (radians)
    void setHorizontal(AmmoniteId cameraId, float newHorizontal) {
      //Find the target camera and update horizontal angle
      if (cameraTrackerMap.contains(cameraId)) {
        cameraTrackerMap[cameraId].horizontalAngle = newHorizontal;
      }
    }

    //Set vertical angle (radians)
    void setVertical(AmmoniteId cameraId, float newVertical) {
      //Find the target camera and update vertical angle
      if (cameraTrackerMap.contains(cameraId)) {
        cameraTrackerMap[cameraId].verticalAngle = newVertical;
      }
    }

    //Set field of view (radians)
    void setFieldOfView(AmmoniteId cameraId, float newFov) {
      //Find the target camera and update field of view
      if (cameraTrackerMap.contains(cameraId)) {
        cameraTrackerMap[cameraId].fov = newFov;
      }
    }
  }
}
