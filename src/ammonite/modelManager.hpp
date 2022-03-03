#ifndef MODELS
#define MODELS

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <GL/glew.h>

namespace ammonite {
  namespace models {
    struct InternalModelData {
      std::vector<glm::vec3> vertices, normals;
      std::vector<glm::vec2> texturePoints;
      std::vector<unsigned int> indices;
      GLuint vertexBufferId;
      GLuint normalBufferId;
      GLuint textureBufferId;
      GLuint elementBufferId;
      int vertexCount;
      int refCount = 1;
    };

    struct InternalModel {
      InternalModelData* data;
      glm::mat4 translationMatrix;
      glm::mat4 rotationMatrix;
      glm::mat4 scaleMatrix;
      GLuint textureId;
      std::string modelName;
      int modelId;
    };

    int createModel(const char* objectPath, bool* externalSuccess);
    InternalModel* getModelPtr(int modelId);
    void deleteModel(int modelId);
  }
}

#endif
