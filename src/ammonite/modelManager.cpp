#include <iostream>
#include <vector>

#include <tiny_obj_loader.h>
#include <glm/glm.hpp>

namespace ammonite {
  namespace models {
    bool loadObject(const char* objectPath,
         std::vector<glm::vec3> &out_vertices,
         std::vector<glm::vec2> &out_uvs,
         std::vector<glm::vec3> &out_normals) {

      tinyobj::ObjReaderConfig reader_config;
      reader_config.mtl_search_path = "./";

      tinyobj::ObjReader reader;

      if (!reader.ParseFromFile(objectPath, reader_config)) {
        if (!reader.Error().empty()) {
          std::cerr << reader.Error();
        }
        return false;
      }

      auto& attrib = reader.GetAttrib();
      auto& shapes = reader.GetShapes();

      // Loop over shapes
      for (size_t s = 0; s < shapes.size(); s++) {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
          size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

          // Loop over vertices in the face.
          for (size_t v = 0; v < fv; v++) {
            //Access vertex
            tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
            tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
            tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
            tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

            glm::vec3 vertex = glm::vec3(vx, vy, vz);
            out_vertices.push_back(vertex);

            // Check if `normal_index` is zero or positive. negative = no normal data
            if (idx.normal_index >= 0) {
              tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
              tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
              tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];

              glm::vec3 normal = glm::vec3(nx, ny, nz);
              out_normals.push_back(normal);
            }

            // Check if `texcoord_index` is zero or positive. negative = no texcoord data
            if (idx.texcoord_index >= 0) {
              tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
              tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];

              glm::vec2 texturePoint = glm::vec2(tx, ty);
              out_uvs.push_back(texturePoint);
            }
          }
          index_offset += fv;
        }
      }
      return true;
    }
  }
}
