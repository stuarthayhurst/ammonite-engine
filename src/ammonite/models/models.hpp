#ifndef INTERNALMODELS
#define INTERNALMODELS

#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

extern "C" {
  #include <epoxy/gl.h>
}

#include "../internal.hpp"
#include "../maths/vector.hpp"
#include "../utils/id.hpp"

//Include public interface
#include "../../include/ammonite/models/models.hpp" // IWYU pragma: export

//Model types, explicitly enumerated since it's also used as an index
enum AMMONITE_INTERNAL ModelTypeEnum : unsigned char {
  AMMONITE_MODEL = 0,
  AMMONITE_LIGHT_EMITTER = 1
};

namespace ammonite {
  namespace models {
    namespace AMMONITE_INTERNAL internal {
      struct ModelLoadInfo {
        std::string modelDirectory;
        bool flipTexCoords;
        bool srgbTextures;
      };

      struct VertexData {
        ammonite::Vec<float, 3> vertex;
        ammonite::Vec<float, 3> normal;
        ammonite::Vec<float, 2> texturePoint;
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
        AmmoniteDrawEnum drawMode = AMMONITE_DRAW_ACTIVE;
        AmmoniteId lightEmitterId = 0;
        unsigned int lightIndex;
        std::string modelName;
        AmmoniteId modelId = 0;
        ModelTypeEnum modelType = AMMONITE_MODEL;
      };

      unsigned int getModelCount(ModelTypeEnum modelType);
      void getModels(ModelTypeEnum modelType, unsigned int modelCount,
                     ModelInfo* modelArr[]);

      ModelInfo* getModelPtr(AmmoniteId modelId);
      bool* getModelsMovedPtr();
      void setLightEmitterId(AmmoniteId modelId, AmmoniteId lightEmitterId);
      AmmoniteId getLightEmitterId(AmmoniteId modelId);

      bool loadObject(const std::string& objectPath,
                      models::internal::ModelData* modelObjectData,
                      const ModelLoadInfo& modelLoadInfo);
    }
  }
}

#endif
