#ifndef INTERNALMODELS
#define INTERNALMODELS

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <GL/glew.h>

/* Internally exposed header:
 - Allow access to model tracker internally
 - Expose data structures for models
*/

namespace ammonite {
  namespace models {
    struct VertexData {
      glm::vec3 vertices, normals;
      glm::vec2 texturePoints;
    };

    struct MeshData {
      std::vector<VertexData> vertexData;
      std::vector<unsigned int> indices;
      GLuint vertexBufferId = 0;
      GLuint elementBufferId = 0;
      GLuint vertexArrayId = 0;
      GLuint textureId = 0;
      int vertexCount = 0;
    };

    struct ModelData {
      int refCount = 1;
      std::vector<MeshData> meshes;
    };

    struct PositionData {
      glm::mat4 modelMatrix;
      glm::mat3 normalMatrix;
      glm::mat4 translationMatrix;
      glm::mat4 scaleMatrix;
      glm::quat rotationQuat;
    };

    struct ModelInfo {
      ModelData* modelData;
      PositionData positionData;
      int drawMode = 0;
      bool active = true;
      bool lightEmitting = false;
      std::string modelName;
      int modelId;
    };

    ModelInfo* getModelPtr(int modelId);
    void setLightEmitting(int modelId, bool lightEmitting);
    bool getLightEmitting(int modelId);
  }
}

#endif
