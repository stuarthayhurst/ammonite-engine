#ifndef INTERNALMODELS
#define INTERNALMODELS

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <GL/glew.h>

namespace ammonite {
  namespace models {
    struct VertexData {
      glm::vec3 vertices, normals;
      glm::vec2 texturePoints;
    };

    struct InternalModelData {
      std::vector<VertexData> modelData;
      std::vector<unsigned int> indices;
      GLuint vertexBufferId = 0;
      GLuint elementBufferId = 0;
      GLuint vertexArrayId = 0;
      int vertexCount = 0;
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
      bool lightEmitting = false;
      std::string modelName;
      int modelId;
    };

    InternalModel* getModelPtr(int modelId);
    void setLightEmitting(int modelId, bool lightEmitting);
    bool getLightEmitting(int modelId);
  }
}

#endif
