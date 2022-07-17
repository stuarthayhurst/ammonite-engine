#ifndef MODELS
#define MODELS

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <GL/glew.h>

namespace ammonite {
  namespace models {
    int createModel(const char* objectPath, bool* externalSuccess);
    int createModel(const char* objectPath, bool flipTexCoords, bool* externalSuccess);
    void deleteModel(int modelId);
    int copyModel(int modelId);

    void applyTexture(int modelId, const char* texturePath, bool* externalSuccess);
    int getVertexCount(int modelId);

    namespace draw {
      void setDrawMode(int modelId, int drawMode);
      void setActive(int modelId, bool active);
    }

    namespace position {
      glm::vec3 getPosition(int modelId);
      glm::vec3 getScale(int modelId);
      glm::vec3 getRotation(int modelId);

      //Absolute movements
      void setPosition(int modelId, glm::vec3 position);
      void setScale(int modelId, glm::vec3 scale);
      void setScale(int modelId, float scaleMultiplier);
      void setRotation(int modelId, glm::vec3 rotation);

      //Relative adjustments
      void translateModel(int modelId, glm::vec3 translation);
      void scaleModel(int modelId, glm::vec3 scale);
      void scaleModel(int modelId, float scaleMultiplier);
      void rotateModel(int modelId, glm::vec3 rotation);
    }
  }
}

#endif
