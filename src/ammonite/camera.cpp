#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include "internal/sharedSettings.hpp"

#ifdef DEBUG
  #include <iostream>
#endif

namespace ammonite {
  namespace camera {
    namespace {
      struct Camera {
        glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
        float horizontalAngle = glm::pi<float>();
        float verticalAngle = 0.0f;
        float fov = 45.0f;
      };

      //View and projection matrices
      glm::mat4 viewMatrix;
      glm::mat4 projectionMatrix;
      float* aspectRatio = ammonite::settings::getAspectRatioPtr();

      //Create and instance of the camera
      Camera camera;
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
        //Vector for current direction faced
        glm::vec3 direction = glm::vec3(
          std::cos(camera.verticalAngle) * std::sin(camera.horizontalAngle),
          std::sin(camera.verticalAngle),
          std::cos(camera.verticalAngle) * std::cos(camera.horizontalAngle)
        );

        //Right vector, relative to the camera
        glm::vec3 right = glm::vec3(
          std::sin(camera.horizontalAngle - glm::half_pi<float>()),
          0,
          std::cos(camera.horizontalAngle - glm::half_pi<float>())
        );
        //Up vector, relative to the camera
        glm::vec3 up = glm::cross(right, direction);

        //Calculate the projection matrix from FoV, aspect ratio and display range
        projectionMatrix = glm::perspective(glm::radians(camera.fov), *aspectRatio, 0.1f, 100.0f);

        //Calculate view matrix from position, where it's looking, and relative up
        viewMatrix = glm::lookAt(camera.position, camera.position + direction, up);
      }
    }

    //Get position
    glm::vec3 getPosition() {
      return camera.position;
    }

    //Get horizontal angle
    float getHorizontal() {
      return camera.horizontalAngle;
    }

    //Get vertical angle
    float getVertical() {
      return camera.verticalAngle;
    }

    //Get field of view
    float getFieldOfView() {
      return camera.fov;
    }

    //Set position
    void setPosition(glm::vec3 newPosition) {
      camera.position = newPosition;
      matrices::calcMatrices();
    }

    //Set horizontal angle
    void setHorizontal(float newHorizontal) {
      camera.horizontalAngle = newHorizontal;
      matrices::calcMatrices();
    }

    //Set vertical angle
    void setVertical(float newVertical) {
      camera.verticalAngle = newVertical;
      matrices::calcMatrices();
    }

    //Set field of view
    void setFieldOfView(float newFov) {
      camera.fov = newFov;
      matrices::calcMatrices();
    }
  }
}
