#include <iostream>
#include <vector>
#include <map>
#include <cstring>
#include <string>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "textures.hpp"

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
      GLuint vertexArrayId;
      int vertexCount;
      int refCount = 1;
    };

    struct PositionData {
      glm::mat4 modelMatrix;
      glm::mat3 normalMatrix;
      glm::mat4 translationMatrix;
      glm::mat4 scaleMatrix;
      glm::quat rotationQuat;
    };

    struct InternalModel {
      InternalModelData* data;
      PositionData positionData;
      GLuint textureId;
      bool wireframe = false;
      bool active = true;
      std::string modelName;
      int modelId;
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
    std::map<int, models::InternalModel> modelTrackerMap;
    std::map<std::string, models::InternalModelData> modelDataMap;
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

    static void createBuffers(models::InternalModelData* modelObjectData) {
      //Create and fill a vertex buffer
      glGenBuffers(1, &modelObjectData->vertexBufferId);
      glBindBuffer(GL_ARRAY_BUFFER, modelObjectData->vertexBufferId);
      glBufferData(GL_ARRAY_BUFFER, modelObjectData->vertices.size() * sizeof(glm::vec3), &modelObjectData->vertices[0], GL_STATIC_DRAW);

      //Create and fill a normal buffer
      glGenBuffers(1, &modelObjectData->normalBufferId);
      glBindBuffer(GL_ARRAY_BUFFER, modelObjectData->normalBufferId);
      glBufferData(GL_ARRAY_BUFFER, modelObjectData->normals.size() * sizeof(glm::vec3), &modelObjectData->normals[0], GL_STATIC_DRAW);

      //Create and fill a texture buffer
      glGenBuffers(1, &modelObjectData->textureBufferId);
      glBindBuffer(GL_ARRAY_BUFFER, modelObjectData->textureBufferId);
      glBufferData(GL_ARRAY_BUFFER, modelObjectData->texturePoints.size() * sizeof(glm::vec2), &modelObjectData->texturePoints[0], GL_STATIC_DRAW);

      //Create and fill an indices buffer
      glGenBuffers(1, &modelObjectData->elementBufferId);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelObjectData->elementBufferId);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, modelObjectData->indices.size() * sizeof(unsigned int), &modelObjectData->indices[0], GL_STATIC_DRAW);

      //Create the vertex attribute buffer
      glGenVertexArrays(1, &modelObjectData->vertexArrayId);
      glBindVertexArray(modelObjectData->vertexArrayId);

      //Vertex attribute buffer
      glBindBuffer(GL_ARRAY_BUFFER, modelObjectData->vertexBufferId);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
      glEnableVertexAttribArray(0);

      //Texture attribute buffer
      glBindBuffer(GL_ARRAY_BUFFER, modelObjectData->textureBufferId);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
      glEnableVertexAttribArray(1);

      //Normal attribute buffer
      glBindBuffer(GL_ARRAY_BUFFER, modelObjectData->normalBufferId);
      glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
      glEnableVertexAttribArray(2);
    }

    static void deleteBuffers(models::InternalModelData* modelObjectData) {
      //Delete created buffers and the VAO
      glDeleteBuffers(1, &modelObjectData->vertexBufferId);
      glDeleteBuffers(1, &modelObjectData->normalBufferId);
      glDeleteBuffers(1, &modelObjectData->textureBufferId);
      glDeleteBuffers(1, &modelObjectData->elementBufferId);
      glDeleteVertexArrays(1, &modelObjectData->vertexArrayId);
    }

    static void indexModel(models::InternalModelData* modelObjectData, modelData* rawModelData) {
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
          modelObjectData->indices.push_back(index);
        } else { //Otherwise, add the new vertex to buffer
          modelObjectData->vertices.push_back(rawModelData->vertices[i]);
          modelObjectData->texturePoints.push_back(rawModelData->texturePoints[i]);
          modelObjectData->normals.push_back(rawModelData->normals[i]);

          index = (unsigned int)modelObjectData->vertices.size() - 1;
          modelObjectData->indices.push_back(index);
          vertexIndexMap[packedVertex] = index;
        }
      }
    }

    static void loadObject(const char* objectPath, models::InternalModelData* modelObjectData, bool* externalSuccess) {
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
            } else {
              rawModelData.texturePoints.push_back(glm::vec2(0.0f, 0.0f));
            }
          }
          index_offset += fv;
        }
      }

      //Fill the index buffer
      modelObjectData->vertexCount = rawModelData.vertices.size();
      indexModel(modelObjectData, &rawModelData);

      return;
    }
  }

  //Exposed model handling methods
  namespace models {
      namespace {
        static void calcModelMatrices(models::InternalModel* modelObject) {
          //Recalculate the model matrix when a component changes
          modelObject->positionData.modelMatrix = modelObject->positionData.translationMatrix * glm::toMat4(modelObject->positionData.rotationQuat) * modelObject->positionData.scaleMatrix;

          //Normal matrix
          modelObject->positionData.normalMatrix = glm::transpose(glm::inverse(modelObject->positionData.modelMatrix));
        }
      }

    int createModel(const char* objectPath, bool* externalSuccess) {
      //Create the model
      InternalModel modelObject;
      modelObject.modelName = std::string(objectPath);

      //Reuse model data if it has already been loaded
      auto it = modelDataMap.find(modelObject.modelName);
      if (it != modelDataMap.end()) {
        modelObject.data = &it->second;
        modelObject.data->refCount += 1;
      } else {
        //Create empty InternalModelData object and add to tracker
        InternalModelData newModelData;
        modelDataMap[modelObject.modelName] = newModelData;
        modelObject.data = &modelDataMap[modelObject.modelName];

        //Fill the model data
        loadObject(objectPath, modelObject.data, externalSuccess);
        createBuffers(modelObject.data);
      }

      PositionData positionData;
      positionData.translationMatrix = glm::mat4(1.0f);
      positionData.scaleMatrix = glm::mat4(1.0f);
      positionData.rotationQuat = glm::quat(glm::vec3(0, 0, 0));

      modelObject.positionData = positionData;

      //Calculate model and normal matrices
      calcModelMatrices(&modelObject);

      //Add model to the tracker and return the ID
      modelObject.modelId = modelTrackerMap.size() + 1;
      modelTrackerMap[modelObject.modelId] = modelObject;
      return modelObject.modelId;
    }

    InternalModel* getModelPtr(int modelId) {
      //Check the model exists, and return a pointer
      auto it = modelTrackerMap.find(modelId);
      if (it != modelTrackerMap.end()) {
        return &it->second;
      } else {
        return nullptr;
      }
    }

    void deleteModel(int modelId) {
      //Check the model actually exists
      auto it = modelTrackerMap.find(modelId);
      if (it != modelTrackerMap.end()) {
        InternalModel* modelObject = &it->second;
        InternalModelData* modelObjectData = modelObject->data;
        //Decrease the reference count of the model data
        modelObjectData->refCount -= 1;

        //If the model's data is now unused, destroy it
        if (modelObjectData->refCount < 1) {
          //Destroy the model's buffers, texture and position in second tracker layer
          deleteBuffers(modelObjectData);
          ammonite::textures::deleteTexture(modelObject->textureId);
          modelDataMap.erase(modelObject->modelName);
        }

        //Remove the model from the tracker
        modelTrackerMap.erase(modelId);
      }
    }

    namespace position {
      void translateModel(int modelId, glm::vec3 translation) {
        //Get the model and translate it
        models::InternalModel* modelObject = models::getModelPtr(modelId);

        //Check the model exists
        if (modelObject == nullptr) {
          return;
        }

        modelObject->positionData.translationMatrix = glm::translate(
          modelObject->positionData.translationMatrix,
          translation);

        //Recalculate model and normal matrices
        calcModelMatrices(modelObject);
      }

      void scaleModel(int modelId, glm::vec3 scaleVector) {
        //Get the model and scale it
        models::InternalModel* modelObject = models::getModelPtr(modelId);

        //Check the model exists
        if (modelObject == nullptr) {
          return;
        }

        modelObject->positionData.scaleMatrix = glm::scale(
          modelObject->positionData.scaleMatrix,
          scaleVector);

        //Recalculate model and normal matrices
        calcModelMatrices(modelObject);
      }

      void scaleModel(int modelId, float scaleMultiplier) {
        scaleModel(modelId, glm::vec3(scaleMultiplier, scaleMultiplier, scaleMultiplier));
      }

      void rotateModel(int modelId, glm::vec3 rotation) {
        //Get the model and rotate it
        models::InternalModel* modelObject = models::getModelPtr(modelId);

        //Check the model exists
        if (modelObject == nullptr) {
          return;
        }

        glm::vec3 rotationRadians = glm::vec3(glm::radians(rotation[0]), glm::radians(rotation[1]), glm::radians(rotation[2]));

        modelObject->positionData.rotationQuat = glm::quat(rotationRadians) * modelObject->positionData.rotationQuat;

        //Recalculate model and normal matrices
        calcModelMatrices(modelObject);
      }
    }
  }
}
