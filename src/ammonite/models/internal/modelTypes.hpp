#ifndef MODELTYPES
#define MODELTYPES

/* Internally exposed header:
 - Define data structures for models
*/

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <GL/glew.h>

#include "../../enums.hpp"

namespace ammonite {
  namespace models {
    namespace internal {
      struct VertexData {
        glm::vec3 vertex, normal;
        glm::vec2 texturePoint;
      };

      struct TextureIdGroup {
        int diffuseId = -1;
        int specularId = -1;
      };

      struct MeshData {
        VertexData* meshData = nullptr;
        int vertexCount = 0;
        unsigned int* indices = nullptr;
        int indexCount = 0;
        GLuint vertexBufferId = 0;
        GLuint elementBufferId = 0;
        GLuint vertexArrayId = 0;
      };

      struct ModelData {
        int refCount = 1;
        std::vector<MeshData> meshes;
        std::vector<TextureIdGroup> textureIds;
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
        std::vector<TextureIdGroup> textureIds;
        AmmoniteEnum drawMode = AMMONITE_DRAW_ACTIVE;
        int lightEmitterId = -1;
        int lightIndex = -1;
        std::string modelName;
        int modelId;
        AmmoniteEnum modelType = AMMONITE_MODEL;
      };
    }
  }
}

#endif
