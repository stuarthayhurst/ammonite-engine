#ifndef MODELS
#define MODELS

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <GL/glew.h>

namespace ammonite {
  namespace models {
    struct InternalModelData {
      std::vector<glm::vec3> vertices, normals;
      std::vector<glm::vec2> texturePoints;
      std::vector<unsigned int> indices;
      GLuint vertexBufferId;
      GLuint normalBufferId;
      GLuint textureBufferId;
      GLuint elementBufferId;
      GLuint vertexArrayId;
      int vertexCount;
      int refCount = 1;
    };

    struct PositionData {
      glm::mat4 modelMatrix;
      glm::mat3 normalMatrix;
      glm::mat4 translationMatrix;
      glm::mat4 scaleMatrix;
      glm::quat rotationQuat;
    };

    struct InternalModel {
      InternalModelData* data;
      PositionData positionData;
      GLuint textureId = 0;
      int drawMode = 0;
      bool active = true;
      std::string modelName;
      int modelId;
    };

    int createModel(const char* objectPath, bool* externalSuccess);
    InternalModel* getModelPtr(int modelId);
    void deleteModel(int modelId);

    void applyTexture(int modelId, const char* texturePath, bool* externalSuccess);
    int getVertexCount(int modelId);

    namespace draw {
      void setDrawMode(int modelId, int drawMode);
      void setActive(int modelId, bool active);
    }

    namespace position {
      void translateModel(int modelId, glm::vec3 translation);
      void scaleModel(int modelId, glm::vec3 scale);
      void scaleModel(int modelId, float scaleMultiplier);
      void rotateModel(int modelId, glm::vec3 rotation);
    }
  }
}

#endif
