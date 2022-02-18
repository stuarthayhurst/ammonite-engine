#ifndef OBJECT
#define OBJECT

namespace ammonite {
  namespace models {
    struct internalModel {
      std::vector<glm::vec3> vertices, normals;
      std::vector<glm::vec2> uvs;
    };

    bool loadObject(const char* objectPath,
         std::vector<glm::vec3> &out_vertices,
         std::vector<glm::vec2> &out_uvs,
         std::vector<glm::vec3> &out_normals);
  }
}

#endif
