#ifndef INTERNALMODELTYPES
#define INTERNALMODELTYPES

#include <string>
#include <unordered_set>
#include <vector>

extern "C" {
  #include <epoxy/gl.h>
}

#include "../maths/matrix.hpp"
#include "../maths/quaternion.hpp"
#include "../utils/id.hpp"
#include "../visibility.hpp"

//Required for AmmoniteDrawEnum, AmmoniteVertex and AmmoniteMaterial
#include "../../include/ammonite/models/models.hpp"

//Model types, explicitly enumerated since it's also used as an index
enum AMMONITE_INTERNAL ModelTypeEnum : unsigned char {
  AMMONITE_MODEL = 0,
  AMMONITE_LIGHT_EMITTER = 1
};

namespace ammonite {
  namespace models {
    namespace AMMONITE_INTERNAL internal {
      /*
       - Store information for each vertex and index in a single mesh
       - Only exists to be uploaded to the GPU
      */
      struct RawMeshData {
        AmmoniteVertex* vertexData = nullptr;
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
        std::string modelKey;
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

      //Data required to load from a file
      //NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
      struct ModelFileInfo {
        const std::string& modelDirectory;
        const std::string& objectPath;
        const bool flipTexCoords;
        const bool srgbTextures;
      };
      //NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)

      //Data required to load from memory
      //NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
      struct ModelMemoryInfo {
        const AmmoniteVertex** meshArray;
        const unsigned int** indicesArray;
        const AmmoniteMaterial* materials;

        const unsigned int meshCount;
        const unsigned int* vertexCounts;
        const unsigned int* indexCounts;
      };
      //NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)

      //Information used to support model loading from various sources
      struct ModelLoadInfo {
        union {
          ModelFileInfo fileInfo;
          ModelMemoryInfo memoryInfo;
        };
        bool isFileBased;
      };
    }
  }
}

#endif
