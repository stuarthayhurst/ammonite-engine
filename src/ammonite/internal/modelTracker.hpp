#ifndef INTERNALMODELS
#define INTERNALMODELS

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
      bool lightEmitting = false;
      std::string modelName;
      int modelId;
    };

    InternalModel* getModelPtr(int modelId);
    void setLightEmitting(int modelId, bool lightEmitting);
    bool setLightEmitting(int modelId);
  }
}

#endif
