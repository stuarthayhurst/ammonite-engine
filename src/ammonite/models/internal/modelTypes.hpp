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
        std::vector<VertexData> meshData;
        std::vector<unsigned int> indices;
        GLuint vertexBufferId = 0; //vertexBufferId and elementBufferId must stay in this memory layout
        GLuint elementBufferId = 0;
        GLuint vertexArrayId = 0;
        int vertexCount = 0;
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
