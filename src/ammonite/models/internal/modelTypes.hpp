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
#include "../../types.hpp"

namespace ammonite {
  namespace models {
    namespace internal {
      struct VertexData {
        glm::vec3 vertex, normal;
        glm::vec2 texturePoint;
      };

      struct TextureIdGroup {
        GLuint diffuseId = 0;
        GLuint specularId = 0;
      };

      struct MeshData {
        VertexData* meshData = nullptr;
        unsigned int vertexCount = 0;
        unsigned int* indices = nullptr;
        unsigned int indexCount = 0;
        GLuint vertexBufferId = 0;
        GLuint elementBufferId = 0;
        GLuint vertexArrayId = 0;
      };

      struct ModelData {
        unsigned int refCount = 1;
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
        AmmoniteId lightEmitterId = 0;
        unsigned int lightIndex;
        std::string modelName;
        AmmoniteId modelId = 0;
        AmmoniteEnum modelType = AMMONITE_MODEL;
      };
    }
  }
}

#endif
