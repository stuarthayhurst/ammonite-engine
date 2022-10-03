#ifndef INTERNALMODELS
#define INTERNALMODELS

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <GL/glew.h>

#include "../constants.hpp"

/* Internally exposed header:
 - Allow access to model tracker internally
 - Expose data structures for models
*/

namespace ammonite {
  namespace models {
    struct VertexData {
      glm::vec3 vertex, normal;
      glm::vec2 texturePoint;
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
      int softRefCount = 0;
      std::vector<MeshData> meshes;
      std::vector<GLuint> textureIds;
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
      std::vector<GLuint> textureIds;
      short drawMode = 0;
      bool isLoaded = true;
      bool isLightEmitting = false;
      std::string modelName;
      int modelId;
      unsigned short modelType = AMMONITE_MODEL;
    };

    int getModelCount(unsigned short modelType);
    void getModels(unsigned short modelType, int modelCount, ModelInfo* modelArr[]);

    ModelInfo* getModelPtr(int modelId);
    void setLightEmitting(int modelId, bool lightEmitting);
    bool getLightEmitting(int modelId);
  }
}

#endif
