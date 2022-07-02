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
#include "internal/modelTracker.hpp"
#include "internal/lightTracker.hpp"
#include "internal/textureTracker.hpp"

namespace ammonite {
  namespace {
    struct PackedVertexInfo {
      glm::vec3 position, normal;
      glm::vec2 texturePoint;
      bool operator<(const PackedVertexInfo that) const {
        return std::memcmp((void*)this, (void*)&that, sizeof(PackedVertexInfo))>0;
      };
    };

    //Track all loaded models
    std::map<int, models::ModelInfo> modelTrackerMap;
    std::map<std::string, models::MeshData> modelDataMap;
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

    static void createBuffers(models::MeshData* modelObjectData) {
      //Create and fill interleaved vertex + texture + normal buffer
      glGenBuffers(1, &modelObjectData->vertexBufferId);
      glBindBuffer(GL_ARRAY_BUFFER, modelObjectData->vertexBufferId);
      glBufferData(GL_ARRAY_BUFFER, modelObjectData->modelData.size() * sizeof(models::VertexData), &modelObjectData->modelData[0], GL_STATIC_DRAW);

      //Create and fill an indices buffer
      glGenBuffers(1, &modelObjectData->elementBufferId);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelObjectData->elementBufferId);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, modelObjectData->indices.size() * sizeof(unsigned int), &modelObjectData->indices[0], GL_STATIC_DRAW);

      //Create the vertex attribute buffer
      glGenVertexArrays(1, &modelObjectData->vertexArrayId);
      glBindVertexArray(modelObjectData->vertexArrayId);
      glBindBuffer(GL_ARRAY_BUFFER, modelObjectData->vertexBufferId);

      //Vertex attribute
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(0));
      glEnableVertexAttribArray(0);

      //Normal attribute
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
      glEnableVertexAttribArray(1);

      //Texture attribute
      glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
      glEnableVertexAttribArray(2);
    }

    static void deleteBuffers(models::MeshData* modelObjectData) {
      //Delete created buffers and the VAO
      glDeleteBuffers(1, &modelObjectData->vertexBufferId);
      glDeleteBuffers(1, &modelObjectData->elementBufferId);
      glDeleteVertexArrays(1, &modelObjectData->vertexArrayId);
    }

    static void indexModel(models::MeshData* modelObjectData, std::vector<models::VertexData>* rawModelData) {
      //Map of known vertices
      std::map<PackedVertexInfo, unsigned int> vertexIndexMap;

      std::vector<models::VertexData>* modelData = &modelObjectData->modelData;

      //Iterate over every vertex, and index them
      for (unsigned int i = 0; i < rawModelData->size(); i++) {
        //Pack vertex information into a struct
        PackedVertexInfo packedVertex = {(*rawModelData)[i].vertices, (*rawModelData)[i].normals, (*rawModelData)[i].texturePoints};

        //Search for an identical vertex
        bool found = false;
        unsigned int index = getIdenticalVertexIndex(&packedVertex, &vertexIndexMap, &found);

        //If the vertex has already been used, reuse the index
        if (found) {
          modelObjectData->indices.push_back(index);
        } else { //Otherwise, add the new vertex to bufferd
          modelData->push_back((*rawModelData)[i]);

          index = (unsigned int)modelObjectData->modelData.size() - 1;
          modelObjectData->indices.push_back(index);
          vertexIndexMap[packedVertex] = index;
        }
      }
    }

    static void loadObject(const char* objectPath, models::MeshData* modelObjectData, bool* externalSuccess) {
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

      //Storage for unindexed model data
      std::vector<models::VertexData> rawModelData;

      //Loop over shapes
      for (size_t s = 0; s < shapes.size(); s++) {
        //Loop over faces
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
          size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

          //Loop each vertex
          for (size_t v = 0; v < fv; v++) {
            models::VertexData currVertex;

            //Vertex
            tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
            tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
            tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
            tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];
            currVertex.vertices = glm::vec3(vx, vy, vz);

            //Normals
            if (idx.normal_index >= 0) {
              tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
              tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
              tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
              currVertex.normals = glm::vec3(nx, ny, nz);
            }

            //Texture points
            if (idx.texcoord_index >= 0) {
              tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
              tinyobj::real_t ty = 1.0f - attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
              currVertex.texturePoints = glm::vec2(tx, ty);
            } else {
              currVertex.texturePoints = glm::vec2(0.0f, 0.0f);
            }

            //Save the vertex
            rawModelData.push_back(currVertex);
          }
          index_offset += fv;
        }
      }

      //Fill the index buffer
      modelObjectData->vertexCount = rawModelData.size();
      indexModel(modelObjectData, &rawModelData);

      return;
    }
  }

  //Internally exposed model handling methods
  namespace models {
    ModelInfo* getModelPtr(int modelId) {
      //Check the model exists, and return a pointer
      auto it = modelTrackerMap.find(modelId);
      if (it != modelTrackerMap.end()) {
        return &it->second;
      } else {
        return nullptr;
      }
    }

    void setLightEmitting(int modelId, bool lightEmitting) {
      ModelInfo* modelPtr = models::getModelPtr(modelId);
      if (modelPtr != nullptr) {
        modelPtr->lightEmitting = lightEmitting;
      }
    }

    bool getLightEmitting(int modelId) {
      ModelInfo* modelPtr = models::getModelPtr(modelId);
      if (modelPtr != nullptr) {
        return modelPtr->lightEmitting;
      }
      return false;
    }
  }

  //Exposed model handling methods
  namespace models {
    namespace {
      static void calcModelMatrices(models::ModelInfo* modelObject) {
        //Recalculate the model matrix when a component changes
        modelObject->positionData.modelMatrix = modelObject->positionData.translationMatrix * glm::toMat4(modelObject->positionData.rotationQuat) * modelObject->positionData.scaleMatrix;

        //Normal matrix
        modelObject->positionData.normalMatrix = glm::transpose(glm::inverse(modelObject->positionData.modelMatrix));
      }

      //Track cumulative number of created models
      int totalModels = 0;
    }

    int createModel(const char* objectPath, bool* externalSuccess) {
      //Create the model
      ModelInfo modelObject;
      modelObject.modelName = std::string(objectPath);

      //Reuse model data if it has already been loaded
      auto it = modelDataMap.find(modelObject.modelName);
      if (it != modelDataMap.end()) {
        modelObject.data = &it->second;
        modelObject.data->refCount += 1;
      } else {
        //Create empty MeshData object and add to tracker
        MeshData newModelData;
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
      modelObject.modelId = ++totalModels;
      modelTrackerMap[modelObject.modelId] = modelObject;
      return modelObject.modelId;
    }

    int copyModel(int modelId) {
      //Get the model and check it exists
      models::ModelInfo* oldModelObject = models::getModelPtr(modelId);
      if (oldModelObject == nullptr) {
        return -1;
      }

      //Copy model data
      ModelInfo modelObject = *oldModelObject;
      modelObject.data->refCount += 1;

      ammonite::textures::copyTexture(modelObject.textureId);

      //Add model to the tracker and return the ID
      modelObject.modelId = ++totalModels;
      modelTrackerMap[modelObject.modelId] = modelObject;
      return modelObject.modelId;
    }

    void deleteModel(int modelId) {
      //Check the model actually exists
      auto it = modelTrackerMap.find(modelId);
      if (it != modelTrackerMap.end()) {
        ModelInfo* modelObject = &it->second;
        MeshData* modelObjectData = modelObject->data;
        //Decrease the reference count of the model data
        modelObjectData->refCount -= 1;

        //Reduce reference count on texture
        ammonite::textures::deleteTexture(modelObject->textureId);

        //If the model data is now unused, destroy it
        if (modelObjectData->refCount < 1) {
          //Destroy the model buffers and position in second tracker layer
          deleteBuffers(modelObjectData);
          modelDataMap.erase(modelObject->modelName);
        }

        //Unlink any attached light source
        ammonite::lighting::unlinkByModel(modelId);

        //Remove the model from the tracker
        modelTrackerMap.erase(modelId);
      }
    }

    void applyTexture(int modelId, const char* texturePath, bool* externalSuccess) {
      ModelInfo* modelPtr = models::getModelPtr(modelId);
      if (modelPtr == nullptr) {
        *externalSuccess = false;
        return;
      }

      //If a texture is already applied, remove it
      if (modelPtr->textureId != 0) {
        ammonite::textures::deleteTexture(modelPtr->textureId);
        modelPtr->textureId = 0;
      }

      //Create new texture and apply to the model
      int textureId = ammonite::textures::loadTexture(texturePath, externalSuccess);
      if (!*externalSuccess) {
        return;
      }
      modelPtr->textureId = textureId;
    }

    //Return the number of vertices on a model
    int getVertexCount(int modelId) {
      ModelInfo* modelPtr = models::getModelPtr(modelId);
      if (modelPtr == nullptr) {
        return 0;
      }

      return modelPtr->data->vertexCount;
    }

    namespace draw {
      void setDrawMode(int modelId, int drawMode) {
        ModelInfo* modelPtr = models::getModelPtr(modelId);
        if (modelPtr != nullptr) {
          modelPtr->drawMode = drawMode;
        }
      }

      void setActive(int modelId, bool active) {
        ModelInfo* modelPtr = models::getModelPtr(modelId);
        if (modelPtr != nullptr) {
          modelPtr->active = active;
        }
      }
    }

    //Return position, scale and rotation of a model
    namespace position {
      glm::vec3 getPosition(int modelId) {
        //Get the model and check it exists
        models::ModelInfo* modelObject = models::getModelPtr(modelId);
        if (modelObject == nullptr) {
          return glm::vec3(0.0f);
        }

        return glm::vec3(modelObject->positionData.translationMatrix * glm::vec4(glm::vec3(0, 0, 0), 1));
      }

      glm::vec3 getScale(int modelId) {
        //Get the model and check it exists
        models::ModelInfo* modelObject = models::getModelPtr(modelId);
        if (modelObject == nullptr) {
          return glm::vec3(0.0f);
        }

        return glm::vec3(modelObject->positionData.scaleMatrix * glm::vec4(glm::vec3(1, 1, 1), 1));
      }

      glm::vec3 getRotation(int modelId) {
        //Get the model and check it exists
        models::ModelInfo* modelObject = models::getModelPtr(modelId);
        if (modelObject == nullptr) {
          return glm::vec3(0.0f);
        }

        return glm::degrees(glm::eulerAngles(modelObject->positionData.rotationQuat));
      }
    }

    //Set absolute position, scale and rotation of models
    namespace position {
      void setPosition(int modelId, glm::vec3 position) {
        //Get the model and check it exists
        models::ModelInfo* modelObject = models::getModelPtr(modelId);
        if (modelObject == nullptr) {
          return;
        }

        //Set the position
        modelObject->positionData.translationMatrix = glm::translate(glm::mat4(1.0f), position);

        //Recalculate model and normal matrices
        calcModelMatrices(modelObject);
      }

      void setScale(int modelId, glm::vec3 scale) {
        //Get the model and check it exists
        models::ModelInfo* modelObject = models::getModelPtr(modelId);
        if (modelObject == nullptr) {
          return;
        }

        //Set the scale
        modelObject->positionData.scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

        //Recalculate model and normal matrices
        calcModelMatrices(modelObject);
      }

      void setScale(int modelId, float scaleMultiplier) {
        setScale(modelId, glm::vec3(scaleMultiplier, scaleMultiplier, scaleMultiplier));
      }

      void setRotation(int modelId, glm::vec3 rotation) {
        //Get the model and check it exists
        models::ModelInfo* modelObject = models::getModelPtr(modelId);
        if (modelObject == nullptr) {
          return;
        }

        //Set the rotation
        glm::vec3 rotationRadians = glm::vec3(glm::radians(rotation[0]),
                                              glm::radians(rotation[1]),
                                              glm::radians(rotation[2]));
        modelObject->positionData.rotationQuat = glm::quat(rotationRadians) * glm::quat(glm::vec3(0, 0, 0));

        //Recalculate model and normal matrices
        calcModelMatrices(modelObject);
      }
    }

    //Translate, scale and rotate models
    namespace position {
      void translateModel(int modelId, glm::vec3 translation) {
        //Get the model and check it exists
        models::ModelInfo* modelObject = models::getModelPtr(modelId);
        if (modelObject == nullptr) {
          return;
        }

        //Translate it
        modelObject->positionData.translationMatrix = glm::translate(
          modelObject->positionData.translationMatrix,
          translation);

        //Recalculate model and normal matrices
        calcModelMatrices(modelObject);
      }

      void scaleModel(int modelId, glm::vec3 scaleVector) {
        //Get the model and check it exists
        models::ModelInfo* modelObject = models::getModelPtr(modelId);
        if (modelObject == nullptr) {
          return;
        }

        //Scale it
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
        //Get the model and check it exists
        models::ModelInfo* modelObject = models::getModelPtr(modelId);
        if (modelObject == nullptr) {
          return;
        }

        //Rotate it
        glm::vec3 rotationRadians = glm::vec3(glm::radians(rotation[0]),
                                              glm::radians(rotation[1]),
                                              glm::radians(rotation[2]));
        modelObject->positionData.rotationQuat = glm::quat(rotationRadians) * modelObject->positionData.rotationQuat;

        //Recalculate model and normal matrices
        calcModelMatrices(modelObject);
      }
    }
  }
}
