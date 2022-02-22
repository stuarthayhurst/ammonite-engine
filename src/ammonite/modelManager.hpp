#ifndef MODELS
#define MODELS

namespace ammonite {
  namespace models {
    struct internalModel {
      std::vector<glm::vec3> vertices, normals;
      std::vector<glm::vec2> texturePoints;
      GLuint vertexBufferId;
      GLuint normalBufferId;
      GLuint textureBufferId;
      GLuint textureId;
      int modelId = 0;
    };

    int createModel(const char* objectPath, bool* externalSuccess);
    internalModel* getModelPtr(int modelId);
    void deleteModel(int modelId);

  }
}

#endif
