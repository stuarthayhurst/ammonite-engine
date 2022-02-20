#ifndef OBJECT
#define OBJECT

namespace ammonite {
  namespace models {
    struct internalModel {
      std::vector<glm::vec3> vertices, normals;
      std::vector<glm::vec2> texturePoints;
      GLuint vertexBufferId;
      GLuint normalBufferId;
      GLuint textureId;
    };

    void createBuffers(internalModel &modelObject);
    bool loadObject(const char* objectPath, internalModel &modelObject);
  }
}

#endif
