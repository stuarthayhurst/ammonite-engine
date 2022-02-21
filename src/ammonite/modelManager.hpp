#ifndef OBJECT
#define OBJECT

namespace ammonite {
  namespace models {
    struct internalModel {
      std::vector<glm::vec3> vertices, normals;
      std::vector<glm::vec2> texturePoints;
      GLuint vertexBufferId;
      GLuint normalBufferId;
      GLuint textureBufferId;
      GLuint textureId;
    };

    void createBuffers(internalModel &modelObject);
    void deleteBuffers(internalModel &modelObject);
    void loadObject(const char* objectPath, internalModel &modelObject, bool* externalSuccess);
  }
}

#endif
