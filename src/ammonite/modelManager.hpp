#ifndef MODELS
#define MODELS

#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>

namespace ammonite {
  namespace models {
    struct InternalModel {
      std::vector<glm::vec3> vertices, normals;
      std::vector<glm::vec2> texturePoints;
      std::vector<unsigned int> indices;
      GLuint vertexBufferId;
      GLuint normalBufferId;
      GLuint textureBufferId;
      GLuint elementBufferId;
      GLuint textureId;
      int vertexCount = 0;
      int modelId = 0;
    };

    int createModel(const char* objectPath, bool* externalSuccess);
    InternalModel* getModelPtr(int modelId);
    void deleteModel(int modelId);

  }
}

#endif
