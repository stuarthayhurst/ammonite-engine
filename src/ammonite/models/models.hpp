#ifndef INTERNALMODELS
#define INTERNALMODELS

#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

extern "C" {
  #include <GL/glew.h>
}

#include "../enums.hpp"
#include "../internal.hpp"
#include "../types.hpp"

//Include public interface
#include "../../include/ammonite/models/models.hpp" // IWYU pragma: export

namespace ammonite {
  namespace models {
    namespace AMMONITE_INTERNAL internal {
      struct ModelLoadInfo {
        std::string modelDirectory;
        bool flipTexCoords;
        bool srgbTextures;
      };

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
        unsigned int refCount = 0;
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

      unsigned int getModelCount(AmmoniteEnum modelType);
      void getModels(AmmoniteEnum modelType, unsigned int modelCount, ModelInfo* modelArr[]);

      ModelInfo* getModelPtr(AmmoniteId modelId);
      bool* getModelsMovedPtr();
      void setLightEmitterId(AmmoniteId modelId, AmmoniteId lightEmitterId);
      AmmoniteId getLightEmitterId(AmmoniteId modelId);

      bool loadObject(std::string objectPath, models::internal::ModelData* modelObjectData,
                      ModelLoadInfo modelLoadInfo);
    }
  }
}

#endif
