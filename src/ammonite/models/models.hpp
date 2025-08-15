#ifndef INTERNALMODELS
#define INTERNALMODELS

#include <string>
#include <vector>

extern "C" {
  #include <epoxy/gl.h>
}

#include "../internal.hpp"
#include "../maths/matrix.hpp"
#include "../maths/quaternion.hpp"
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
        ammonite::Mat<float, 4> modelMatrix = {{0}};
        ammonite::Mat<float, 3> normalMatrix = {{0}};
        ammonite::Mat<float, 4> translationMatrix = {{0}};
        ammonite::Mat<float, 4> scaleMatrix = {{0}};
        ammonite::Mat<float, 4> rotationMatrix = {{0}};
        ammonite::Quat<float> rotationQuat = {{0}};
      };

      struct ModelInfo {
        //Mesh, position and texture info
        ModelData* modelData;
        PositionData positionData;
        std::vector<TextureIdGroup> textureIds;

        //Model identification
        std::string modelName;
        AmmoniteId modelId = 0;

        //Model selection factors
        ModelTypeEnum modelType = AMMONITE_MODEL;
        AmmoniteDrawEnum drawMode = AMMONITE_DRAW_ACTIVE;
        AmmoniteId lightEmitterId = 0;
        unsigned int lightIndex;
      };

      struct ModelLoadInfo {
        std::string modelDirectory;
        bool flipTexCoords;
        bool srgbTextures;
      };

      //Model data storage management
      std::string getModelName(const std::string& objectPath,
                               const ModelLoadInfo& modelLoadInfo);
      ModelData* addModelData(const std::string& objectPath,
                              const ModelLoadInfo& modelLoadInfo);
      ModelData* copyModelData(const std::string& modelName);
      bool deleteModelData(const std::string& modelName);

      //Model info retrieval
      unsigned int getModelCount(ModelTypeEnum modelType);
      void getModels(ModelTypeEnum modelType, unsigned int modelCount,
                     ModelInfo* modelArr[]);
      ModelInfo* getModelPtr(AmmoniteId modelId);
      bool* getModelsMovedPtr();

      void setLightEmitterId(AmmoniteId modelId, AmmoniteId lightEmitterId);
      AmmoniteId getLightEmitterId(AmmoniteId modelId);

      //Model loading management
      bool loadObject(const std::string& objectPath, ModelData* modelObjectData,
                      const ModelLoadInfo& modelLoadInfo);
    }
  }
}

#endif
