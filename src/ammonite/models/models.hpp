#ifndef INTERNALMODELS
#define INTERNALMODELS

#include <string>
#include <string_view>
#include <unordered_set>
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
      //Store information for a single vertex
      struct VertexData {
        ammonite::Vec<float, 3> vertex;
        ammonite::Vec<float, 3> normal;
        ammonite::Vec<float, 2> texturePoint;
      };

      /*
       - Store information for each vertex and index in a single mesh
       - Only exists to be uploaded to the GPU
      */
      struct RawMeshData {
        VertexData* vertexData = nullptr;
        unsigned int vertexCount = 0;
        unsigned int* indices = nullptr;
        unsigned int indexCount = 0;
      };

      //Store rendering information on an uploaded mesh
      struct MeshInfoGroup {
        unsigned int vertexCount = 0;
        unsigned int indexCount = 0;
        GLuint vertexBufferId = 0;
        GLuint elementBufferId = 0;
        GLuint vertexArrayId = 0;
      };

      //Store texture IDs for a single mesh
      struct TextureIdGroup {
        GLuint diffuseId = 0;
        GLuint specularId = 0;
      };

      /*
       - Store information for all meshes in a unique model
       - Includes uploaded mesh information, texture information and an ID for
         every model that uses the data
      */
      struct ModelData {
        unsigned int refCount = 0;
        std::string modelName;
        std::vector<MeshInfoGroup> meshInfo;
        std::vector<TextureIdGroup> textureIds;
        std::unordered_set<AmmoniteId> activeModelIds;
        std::unordered_set<AmmoniteId> inactiveModelIds;
      };

      //Store rotation, scale and position information for an instance of a model
      struct PositionData {
        ammonite::Mat<float, 4> modelMatrix = {{0}};
        ammonite::Mat<float, 3> normalMatrix = {{0}};
        ammonite::Mat<float, 4> translationMatrix = {{0}};
        ammonite::Mat<float, 4> scaleMatrix = {{0}};
        ammonite::Mat<float, 4> rotationMatrix = {{0}};
        ammonite::Quat<float> rotationQuat = {{0}};
      };

      /*
       - Store information for an instance of a model
       - Multiple instances can share a ModelData
       - Each instance has a unique ID and PositionData
       - Each instance defaults to the model-provided textures, but can be overridden
      */
      struct ModelInfo {
        //Mesh, position and texture info
        ModelData* modelData;
        PositionData positionData;
        std::vector<TextureIdGroup> textureIds;

        //Model identification
        AmmoniteId modelId = 0;

        //Model selection factors
        ModelTypeEnum modelType = AMMONITE_MODEL;
        AmmoniteDrawEnum drawMode = AMMONITE_DRAW_ACTIVE;
        AmmoniteId lightEmitterId = 0;
        unsigned int lightIndex;
      };

      //Information used to support model loading
      struct ModelLoadInfo {
        std::string modelDirectory;
        bool flipTexCoords;
        bool srgbTextures;
      };

      //Model data storage management
      std::string getModelName(const std::string& objectPath,
                               const ModelLoadInfo& modelLoadInfo);
      ModelData* addModelData(const std::string& objectPath,
                              const ModelLoadInfo& modelLoadInfo,
                              AmmoniteId modelId);
      ModelData* copyModelData(const std::string& modelName, AmmoniteId modelId);
      ModelData* getModelData(const std::string& modelName);
      ModelData* getModelData(std::string_view modelName);
      bool deleteModelData(const std::string& modelName, AmmoniteId modelId);
      void setModelInfoActive(AmmoniteId modelId, bool active);

      //Model info retrieval, by model name
      unsigned int getModelNameCount();
      unsigned int getModelInfoCount(std::string_view modelName);
      void getModelNames(std::string_view modelNameArray[]);
      void getModelInfos(std::string_view modelName, ModelInfo* modelInfoArray[]);

      //Model info retrieval
      unsigned int getModelCount(ModelTypeEnum modelType);
      void getModels(ModelTypeEnum modelType, unsigned int modelCount,
                     ModelInfo* modelInfoArray[]);
      ModelInfo* getModelPtr(AmmoniteId modelId);
      bool* getModelsMovedPtr();

      void setLightEmitterId(AmmoniteId modelId, AmmoniteId lightEmitterId);
      AmmoniteId getLightEmitterId(AmmoniteId modelId);

      //Model loading management
      bool loadObject(const std::string& objectPath, ModelData* modelData,
                      std::vector<RawMeshData>* rawMeshDataVec,
                      const ModelLoadInfo& modelLoadInfo);
    }
  }
}

#endif
