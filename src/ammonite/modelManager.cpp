#include <iostream>
#include <vector>
#include <map>
#include <cstring>

#include <tiny_obj_loader.h>
#include <GL/glew.h>
#include <glm/glm.hpp>

#include "textures.hpp"

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
  }

  namespace {
    struct modelData {
      std::vector<glm::vec3> vertices, normals;
      std::vector<glm::vec2> texturePoints;
      std::vector<unsigned int> indices;
    };

    struct PackedVertexInfo {
      glm::vec3 position;
      glm::vec2 uv;
      glm::vec3 normal;
      bool operator<(const PackedVertexInfo that) const{
        return std::memcmp((void*)this, (void*)&that, sizeof(PackedVertexInfo))>0;
      };
    };

    //Track all loaded models
    std::vector<models::InternalModel> modelTracker(0);
  }

  namespace {
    static unsigned int getIdenticalVertexIndex(PackedVertexInfo* packed, std::map<PackedVertexInfo, unsigned int>* vertexIndexMap, bool* found) {
      //Look for an identical vertex
      std::map<PackedVertexInfo, unsigned int>::iterator it = vertexIndexMap->find(*packed);
      if (it == vertexIndexMap->end()) {
        //No vertex was found
        *found = false;
        return 0;
      } else {
        //A vertex was found, return its index
        *found = true;
        return it->second;
      }
    }

    static void createBuffers(models::InternalModel* modelObject) {
      //Create and fill a vertex buffer
      glGenBuffers(1, &modelObject->vertexBufferId);
      glBindBuffer(GL_ARRAY_BUFFER, modelObject->vertexBufferId);
      glBufferData(GL_ARRAY_BUFFER, modelObject->vertices.size() * sizeof(glm::vec3), &modelObject->vertices[0], GL_STATIC_DRAW);

      //Create and fill a normal buffer
      glGenBuffers(1, &modelObject->normalBufferId);
      glBindBuffer(GL_ARRAY_BUFFER, modelObject->normalBufferId);
      glBufferData(GL_ARRAY_BUFFER, modelObject->normals.size() * sizeof(glm::vec3), &modelObject->normals[0], GL_STATIC_DRAW);

      //Create and fill a texture buffer
      glGenBuffers(1, &modelObject->textureBufferId);
      glBindBuffer(GL_ARRAY_BUFFER, modelObject->textureBufferId);
      glBufferData(GL_ARRAY_BUFFER, modelObject->texturePoints.size() * sizeof(glm::vec2), &modelObject->texturePoints[0], GL_STATIC_DRAW);

      //Create and fill an indices buffer
      glGenBuffers(1, &modelObject->elementBufferId);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelObject->elementBufferId);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, modelObject->indices.size() * sizeof(unsigned int), &modelObject->indices[0], GL_STATIC_DRAW);
    }

    static void deleteBuffers(models::InternalModel* modelObject) {
      //Delete created buffers
      glDeleteBuffers(1, &modelObject->vertexBufferId);
      glDeleteBuffers(1, &modelObject->normalBufferId);
      glDeleteBuffers(1, &modelObject->textureBufferId);
      glDeleteBuffers(1, &modelObject->elementBufferId);
    }

    static void indexModel(models::InternalModel* modelObject, modelData* rawModelData) {
      //Map of known vertices
      std::map<PackedVertexInfo, unsigned int> vertexIndexMap;

      //Iterate over every vertex, and index them
      for (unsigned int i = 0; i < rawModelData->vertices.size(); i++) {
        //Pack vertex information into a struct
        PackedVertexInfo packedVertex = {rawModelData->vertices[i], rawModelData->texturePoints[i], rawModelData->normals[i]};

        //Search for an identical vertex
        bool found;
        unsigned int index = getIdenticalVertexIndex(&packedVertex, &vertexIndexMap, &found);

        //If the vertex has already been used, reuse the index
        if (found) {
          modelObject->indices.push_back(index);
        } else { //Otherwise, add the new vertex to buffer
          modelObject->vertices.push_back(rawModelData->vertices[i]);
          modelObject->texturePoints.push_back(rawModelData->texturePoints[i]);
          modelObject->normals.push_back(rawModelData->normals[i]);

          index = (unsigned int)modelObject->vertices.size() - 1;
          modelObject->indices.push_back(index);
          vertexIndexMap[packedVertex] = index;
        }
      }
    }

    static void loadObject(const char* objectPath, models::InternalModel* modelObject, bool* externalSuccess) {
      tinyobj::ObjReaderConfig reader_config;
      tinyobj::ObjReader reader;

      //Atempt to parse the object
      if (!reader.ParseFromFile(objectPath, reader_config)) {
        if (!reader.Error().empty()) {
          std::cerr << reader.Error();
        }
        *externalSuccess = false;
        return;
      }

      auto& attrib = reader.GetAttrib();
      auto& shapes = reader.GetShapes();

      //Temporary storage for unindexed model data
      modelData rawModelData;

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
            rawModelData.vertices.push_back(vertex);

            //Normals
            if (idx.normal_index >= 0) {
              tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
              tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
              tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];

              glm::vec3 normal = glm::vec3(nx, ny, nz);
              rawModelData.normals.push_back(normal);
            }

            //Texture points
            if (idx.texcoord_index >= 0) {
              tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
              tinyobj::real_t ty = 1.0f - attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];

              glm::vec2 texturePoint = glm::vec2(tx, ty);
              rawModelData.texturePoints.push_back(texturePoint);
            }
          }
          index_offset += fv;
        }
      }

      //Fill the index buffer
      modelObject->vertexCount = rawModelData.vertices.size();
      indexModel(modelObject, &rawModelData);

      return;
    }
  }

  //Exposed model handling methods
  namespace models {
    int createModel(const char* objectPath, bool* externalSuccess) {
      //Create the model
      InternalModel modelObject;
      loadObject(objectPath, &modelObject, externalSuccess);
      createBuffers(&modelObject);

      //Add model to the tracker and return the ID
      modelObject.modelId = modelTracker.size() + 1;
      modelTracker.push_back(modelObject);
      return modelObject.modelId;
    }

    InternalModel* getModelPtr(int modelId) {
      for (long unsigned int i = 0; i < modelTracker.size(); i++) {
        if (modelTracker[i].modelId == modelId) {
          return &modelTracker[i];
        }
      }
      return nullptr;
    }

    void deleteModel(int modelId) {
      //Find the model in modelTracker
      for (long unsigned int i = 0; i < modelTracker.size(); i++) {
        if (modelTracker[i].modelId == modelId) {
          //Destroy the model's buffers and texture
          deleteBuffers(&modelTracker[i]);
          ammonite::textures::deleteTexture(modelTracker[i].textureId);

          //Remove the model from the tracker
          modelTracker.erase(std::next(modelTracker.begin(), i));

          //Exit early
          return;
        }
      }
    }
  }
}
