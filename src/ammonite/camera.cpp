#include <map>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include "internal/internalSettings.hpp"

#ifdef DEBUG
  #include <iostream>
#endif

namespace ammonite {
  namespace camera {
    namespace {
      struct Camera {
        glm::vec3 position = glm::vec3(0.0f);
        float horizontalAngle = glm::pi<float>();
        float verticalAngle = 0.0f;
        float fov = 45.0f;
      } camera;

      //View and projection matrices
      glm::mat4 viewMatrix;
      glm::mat4 projectionMatrix;
      float* aspectRatio = ammonite::settings::runtime::internal::getAspectRatioPtr();

      //Create map to track cameras, with default camera
      int totalCameras = 1;
      int activeCameraId = 0;
      std::map<int, Camera> cameraTrackerMap = {{0, camera}};
    }

    //Pointer methods exposed internally
    namespace matrices {
      glm::mat4* getViewMatrixPtr() {
        return &viewMatrix;
      }

      glm::mat4* getProjectionMatrixPtr() {
        return &projectionMatrix;
      }

      void calcMatrices() {
        //Get the active camera
        Camera activeCamera = cameraTrackerMap[activeCameraId];

        //Vector for current direction faced
        glm::vec3 direction = glm::vec3(
          std::cos(activeCamera.verticalAngle) * std::sin(activeCamera.horizontalAngle),
          std::sin(activeCamera.verticalAngle),
          std::cos(activeCamera.verticalAngle) * std::cos(activeCamera.horizontalAngle)
        );

        //Right vector, relative to the camera
        glm::vec3 right = glm::vec3(
          std::sin(activeCamera.horizontalAngle - glm::half_pi<float>()),
          0,
          std::cos(activeCamera.horizontalAngle - glm::half_pi<float>())
        );
        //Up vector, relative to the camera
        glm::vec3 up = glm::cross(right, direction);

        //Calculate the projection matrix from FoV, aspect ratio and display range
        projectionMatrix = glm::perspective(glm::radians(activeCamera.fov), *aspectRatio, 0.1f, 100.0f);

        //Calculate view matrix from position, where it's looking, and relative up
        viewMatrix = glm::lookAt(activeCamera.position, activeCamera.position + direction, up);
      }
    }

    int getActiveCamera() {
      return activeCameraId;
    }

    void setActiveCamera(int cameraId) {
      //If the camera exists, set as active
      auto it = cameraTrackerMap.find(cameraId);
      if (it != cameraTrackerMap.end()) {
        activeCameraId = cameraId;
        matrices::calcMatrices();
      }
    }

    int createCamera() {
      //Get an ID for the new camera
      int cameraId = totalCameras;
      totalCameras++;

      //Add the new camera to the tracker
      Camera newCamera;
      cameraTrackerMap[cameraId] = newCamera;

      return cameraId;
    }

    void deleteCamera(int cameraId) {
      //Delete the camera if present
      auto it = cameraTrackerMap.find(cameraId);
      if (it != cameraTrackerMap.end() and cameraId != 0) {
        cameraTrackerMap.erase(cameraId);
      }

      //If the deleted camera was the active camera, reset
      setActiveCamera(0);
    }

    //Get position
    glm::vec3 getPosition(int cameraId) {
      auto it = cameraTrackerMap.find(cameraId);
      if (it != cameraTrackerMap.end()) {
        return it->second.position;
      } else {
        return glm::vec3(0.0f);
      }
    }

    //Get horizontal angle (radians)
    float getHorizontal(int cameraId) {
      auto it = cameraTrackerMap.find(cameraId);
      if (it != cameraTrackerMap.end()) {
        return it->second.horizontalAngle;
      } else {
        return 0.0f;
      }
    }

    //Get vertical angle (radians)
    float getVertical(int cameraId) {
      auto it = cameraTrackerMap.find(cameraId);
      if (it != cameraTrackerMap.end()) {
        return it->second.verticalAngle;
      } else {
        return 0.0f;
      }
    }

    //Get field of view
    float getFieldOfView(int cameraId) {
      auto it = cameraTrackerMap.find(cameraId);
      if (it != cameraTrackerMap.end()) {
        return it->second.fov;
      } else {
        return 45.0f;
      }
    }

    //Set position
    void setPosition(int cameraId, glm::vec3 newPosition) {
      //Find the target camera and update position
      auto it = cameraTrackerMap.find(cameraId);
      if (it != cameraTrackerMap.end()) {
        it->second.position = newPosition;
        matrices::calcMatrices();
      }
    }

    //Set horizontal angle (radians)
    void setHorizontal(int cameraId, float newHorizontal) {
      //Find the target camera and update horizontal angle
      auto it = cameraTrackerMap.find(cameraId);
      if (it != cameraTrackerMap.end()) {
        it->second.horizontalAngle = newHorizontal;
        matrices::calcMatrices();
      }
    }

    //Set vertical angle (radians)
    void setVertical(int cameraId, float newVertical) {
      //Find the target camera and update vertical angle
      auto it = cameraTrackerMap.find(cameraId);
      if (it != cameraTrackerMap.end()) {
        it->second.verticalAngle = newVertical;
        matrices::calcMatrices();
      }
    }

    //Set field of view
    void setFieldOfView(int cameraId, float newFov) {
      //Find the target camera and update field of view
      auto it = cameraTrackerMap.find(cameraId);
      if (it != cameraTrackerMap.end()) {
        it->second.fov = newFov;
        matrices::calcMatrices();
      }
    }
  }
}
