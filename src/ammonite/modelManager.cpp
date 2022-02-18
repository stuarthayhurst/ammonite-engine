#include <iostream>
#include <vector>

#include <tiny_obj_loader.h>
#include <glm/glm.hpp>

namespace ammonite {
  namespace models {
    struct internalModel {
      std::vector<glm::vec3> vertices, normals;
      std::vector<glm::vec2> uvs;
    };
  }

  namespace models {
    bool loadObject(const char* objectPath, internalModel &modelObject) {
      tinyobj::ObjReaderConfig reader_config;
      tinyobj::ObjReader reader;

      //Atempt to parse the object
      if (!reader.ParseFromFile(objectPath, reader_config)) {
        if (!reader.Error().empty()) {
          std::cerr << reader.Error();
        }
        return false;
      }

      auto& attrib = reader.GetAttrib();
      auto& shapes = reader.GetShapes();

      //Loop over shapes
      for (size_t s = 0; s < shapes.size(); s++) {
        //Loop over faces
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
          size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

          //Loop each vertex
          for (size_t v = 0; v < fv; v++) {
            //Vertex
            tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
            tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
            tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
            tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

            glm::vec3 vertex = glm::vec3(vx, vy, vz);
            modelObject.vertices.push_back(vertex);

            //Normals
            if (idx.normal_index >= 0) {
              tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
              tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
              tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];

              glm::vec3 normal = glm::vec3(nx, ny, nz);
              modelObject.normals.push_back(normal);
            }

            //Texture points
            if (idx.texcoord_index >= 0) {
              tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
              tinyobj::real_t ty = 1.0f - attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];

              glm::vec2 texturePoint = glm::vec2(tx, ty);
              modelObject.uvs.push_back(texturePoint);
            }
          }
          index_offset += fv;
        }
      }
      return true;
    }
  }
}
