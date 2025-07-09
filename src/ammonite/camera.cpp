#include <cmath>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include "camera.hpp"

#include "graphics/renderer.hpp"
#include "maths/angle.hpp"
#include "utils/id.hpp"
#include "window/window.hpp"

namespace ammonite {
  namespace camera {
    namespace {
      struct Camera {
        glm::vec3 position = glm::vec3(0.0f);
        double horizontalAngle = ammonite::pi<double>();
        double verticalAngle = 0.0f;
        float fov = ammonite::pi<float>() / 4.0f;
      } defaultCamera;

      //View and projection matrices
      glm::mat4 viewMatrix;
      glm::mat4 projectionMatrix;

      //Create map to track cameras, with default camera
      AmmoniteId lastCameraId = 1;
      AmmoniteId activeCameraId = 1;
      std::unordered_map<AmmoniteId, Camera> cameraTrackerMap = {{1, defaultCamera}};
    }

    namespace {
      glm::vec3 calculateDirection(double horizontal, double vertical) {
        return glm::normalize(glm::vec3(
          std::cos(vertical) * std::sin(horizontal),
          std::sin(vertical),
          std::cos(vertical) * std::cos(horizontal)
        ));
      }

      double calculateVerticalAngle(glm::vec3 direction) {
        return std::asin(glm::normalize(direction).y);
      }

      double calculateHorizontalAngle(glm::vec3 direction) {
        direction.y = 0.0f;
        direction = glm::normalize(direction);
        return std::atan2(direction.x, direction.z);
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
        const Camera* activeCamera = &cameraTrackerMap[activeCameraId];

        //Calculate the direction vector
        const glm::vec3 direction = calculateDirection(activeCamera->horizontalAngle,
                                                       activeCamera->verticalAngle);

        //Right vector, relative to the camera
        const glm::vec3 right = glm::vec3(
          std::sin(activeCamera->horizontalAngle - (ammonite::pi<double>() / 2.0)),
          0,
          std::cos(activeCamera->horizontalAngle - (ammonite::pi<double>() / 2.0))
        );

        //Up vector, relative to the camera
        const glm::vec3 up = glm::cross(right, direction);

        //Calculate the projection matrix from FoV, aspect ratio and display range
        const float aspectRatio = ammonite::window::internal::getGraphicsAspectRatio();
        const float renderFarPlane = ammonite::renderer::settings::getRenderFarPlane();
        projectionMatrix = glm::perspective(activeCamera->fov,
                                            aspectRatio, 0.1f, renderFarPlane);

        //Calculate view matrix from position, where it's looking, and relative up
        viewMatrix = glm::lookAt(activeCamera->position,
                                 activeCamera->position + direction, up);
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
        const Camera& activeCamera = cameraTrackerMap[cameraId];
        return calculateDirection(activeCamera.horizontalAngle, activeCamera.verticalAngle);
      }

      return glm::vec3(0.0f);
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
    void setPosition(AmmoniteId cameraId, glm::vec3 position) {
      //Find the target camera and update position
      if (cameraTrackerMap.contains(cameraId)) {
        cameraTrackerMap[cameraId].position = position;
      }
    }

    //Set direction
    void setDirection(AmmoniteId cameraId, glm::vec3 direction) {
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
  }
}
