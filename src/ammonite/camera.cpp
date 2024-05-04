#include <map>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include "graphics/internal/internalRenderCore.hpp"

#include "core/windowManager.hpp"

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
      int totalUserCameras = 0;
      int activeCameraId = 0;
      std::map<int, Camera> cameraTrackerMap = {{0, defaultCamera}};
    }

    namespace {
      static glm::vec3 calculateDirection(double vertical, double horizontal) {
        return glm::vec3(
          glm::cos(vertical) * glm::sin(horizontal),
          glm::sin(vertical),
          glm::cos(vertical) * glm::cos(horizontal)
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
        Camera activeCamera = cameraTrackerMap[activeCameraId];

        //Vector for current direction faced
        glm::vec3 direction = calculateDirection(activeCamera.verticalAngle,
                                                 activeCamera.horizontalAngle);

        //Right vector, relative to the camera
        glm::vec3 right = glm::vec3(
          glm::sin(activeCamera.horizontalAngle - glm::half_pi<float>()),
          0,
          glm::cos(activeCamera.horizontalAngle - glm::half_pi<float>())
        );
        //Up vector, relative to the camera
        glm::vec3 up = glm::cross(right, direction);

        //Calculate the projection matrix from FoV, aspect ratio and display range
        float aspectRatio = ammonite::window::internal::getAspectRatio();
        projectionMatrix = glm::perspective(activeCamera.fov,
                                            aspectRatio, 0.1f, *renderFarPlanePtr);

        //Calculate view matrix from position, where it's looking, and relative up
        viewMatrix = glm::lookAt(activeCamera.position, activeCamera.position + direction, up);
      }
    }

    int getActiveCamera() {
      return activeCameraId;
    }

    void setActiveCamera(int cameraId) {
      //If the camera exists, set as active
      if (cameraTrackerMap.contains(cameraId)) {
        activeCameraId = cameraId;
      }
    }

    int createCamera() {
      //Get an ID for the new camera
      totalUserCameras++;
      int cameraId = totalUserCameras;

      //Add the new camera to the tracker
      Camera newCamera;
      cameraTrackerMap[cameraId] = newCamera;

      return cameraId;
    }

    void deleteCamera(int cameraId) {
      //Delete the camera if present
      if (cameraTrackerMap.contains(cameraId) and cameraId != 0) {
        cameraTrackerMap.erase(cameraId);
      }

      //If the deleted camera was the active camera, reset
      setActiveCamera(0);
    }

    //Get position
    glm::vec3 getPosition(int cameraId) {
      if (cameraTrackerMap.contains(cameraId)) {
        return cameraTrackerMap[cameraId].position;
      } else {
        return glm::vec3(0.0f);
      }
    }

    glm::vec3 getDirection(int cameraId) {
      if (cameraTrackerMap.contains(cameraId)) {
        Camera* activeCamera = &cameraTrackerMap[cameraId];
        glm::vec3 direction = calculateDirection(activeCamera->verticalAngle,
                                                 activeCamera->horizontalAngle);
        return glm::normalize(direction);
      } else {
        return glm::vec3(0.0f);
      }
    }

    //Get horizontal angle (radians)
    float getHorizontal(int cameraId) {
      if (cameraTrackerMap.contains(cameraId)) {
        return cameraTrackerMap[cameraId].horizontalAngle;
      } else {
        return 0.0f;
      }
    }

    //Get vertical angle (radians)
    float getVertical(int cameraId) {
      if (cameraTrackerMap.contains(cameraId)) {
        return cameraTrackerMap[cameraId].verticalAngle;
      } else {
        return 0.0f;
      }
    }

    //Get field of view (radians)
    float getFieldOfView(int cameraId) {
      if (cameraTrackerMap.contains(cameraId)) {
        return cameraTrackerMap[cameraId].fov;
      } else {
        return glm::quarter_pi<float>();
      }
    }

    //Set position
    void setPosition(int cameraId, glm::vec3 newPosition) {
      //Find the target camera and update position
      if (cameraTrackerMap.contains(cameraId)) {
        cameraTrackerMap[cameraId].position = newPosition;
      }
    }

    //Set horizontal angle (radians)
    void setHorizontal(int cameraId, float newHorizontal) {
      //Find the target camera and update horizontal angle
      if (cameraTrackerMap.contains(cameraId)) {
        cameraTrackerMap[cameraId].horizontalAngle = newHorizontal;
      }
    }

    //Set vertical angle (radians)
    void setVertical(int cameraId, float newVertical) {
      //Find the target camera and update vertical angle
      if (cameraTrackerMap.contains(cameraId)) {
        cameraTrackerMap[cameraId].verticalAngle = newVertical;
      }
    }

    //Set field of view (radians)
    void setFieldOfView(int cameraId, float newFov) {
      //Find the target camera and update field of view
      if (cameraTrackerMap.contains(cameraId)) {
        cameraTrackerMap[cameraId].fov = newFov;
      }
    }
  }
}
