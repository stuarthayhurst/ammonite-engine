#include <cstddef>
#include <map>
#include <string>

#include "models.hpp"

#include "../graphics/textures.hpp"

//TODO: Move to the graphics system
namespace {
  void createBuffers(ammonite::models::internal::ModelData* modelObjectData) {
    //Generate buffers for every mesh
    for (unsigned int i = 0; i < modelObjectData->meshes.size(); i++) {
      ammonite::models::internal::MeshData* meshData = &modelObjectData->meshes[i];

      //Create vertex and index buffers
      glCreateBuffers(1, &meshData->vertexBufferId);
      glCreateBuffers(1, &meshData->elementBufferId);

      //Fill interleaved vertex + normal + texture buffer and index buffer
      glNamedBufferData(meshData->vertexBufferId,
                        meshData->vertexCount * (long)sizeof(ammonite::models::internal::VertexData),
                        &meshData->meshData[0], GL_STATIC_DRAW);
      glNamedBufferData(meshData->elementBufferId,
                        meshData->indexCount * (long)sizeof(unsigned int),
                        &meshData->indices[0], GL_STATIC_DRAW);

      //Destroy mesh data early
      delete [] meshData->meshData;
      delete [] meshData->indices;
      meshData->meshData = nullptr;
      meshData->indices = nullptr;

      //Create the vertex attribute buffer
      glCreateVertexArrays(1, &meshData->vertexArrayId);

      const GLuint vaoId = meshData->vertexArrayId;
      const GLuint vboId = meshData->vertexBufferId;
      const int stride = sizeof(ammonite::models::internal::VertexData);

      //Vertex attribute
      glEnableVertexArrayAttrib(vaoId, 0);
      glVertexArrayVertexBuffer(vaoId, 0, vboId,
                                offsetof(ammonite::models::internal::VertexData, vertex), stride);
      glVertexArrayAttribFormat(vaoId, 0, 3, GL_FLOAT, GL_FALSE, 0);
      glVertexArrayAttribBinding(vaoId, 0, 0);

      //Normal attribute
      glEnableVertexArrayAttrib(vaoId, 1);
      glVertexArrayVertexBuffer(vaoId, 1, vboId,
                                offsetof(ammonite::models::internal::VertexData, normal), stride);
      glVertexArrayAttribFormat(vaoId, 1, 3, GL_FLOAT, GL_FALSE, 0);
      glVertexArrayAttribBinding(vaoId, 1, 1);

      //Texture attribute
      glEnableVertexArrayAttrib(vaoId, 2);
      glVertexArrayVertexBuffer(vaoId, 2, vboId,
                                offsetof(ammonite::models::internal::VertexData, texturePoint), stride);
      glVertexArrayAttribFormat(vaoId, 2, 2, GL_FLOAT, GL_FALSE, 0);
      glVertexArrayAttribBinding(vaoId, 2, 2);

      //Element buffer
      glVertexArrayElementBuffer(vaoId, meshData->elementBufferId);
    }
  }

  void deleteBuffers(ammonite::models::internal::ModelData* modelObjectData) {
    //Delete created buffers and the VAO
    for (unsigned int i = 0; i < modelObjectData->meshes.size(); i++) {
      const ammonite::models::internal::MeshData* meshData = &modelObjectData->meshes[i];

      glDeleteBuffers(1, &meshData->vertexBufferId);
      glDeleteBuffers(1, &meshData->elementBufferId);
      glDeleteVertexArrays(1, &meshData->vertexArrayId);
    }
  }
}

namespace ammonite {
  namespace models {
    namespace internal {
      namespace {
        std::map<std::string, ModelData> modelNameDataMap;
      }

      std::string getModelName(const std::string& objectPath,
                               const ModelLoadInfo& modelLoadInfo) {
        const unsigned char extraData = ((int)modelLoadInfo.flipTexCoords << 0) |
                                        ((int)modelLoadInfo.srgbTextures << 1);
        return objectPath + std::to_string(extraData);
      }

      ModelData* addModelData(const std::string& objectPath,
                              const ModelLoadInfo& modelLoadInfo) {
        //If the model has already been loaded, update counter and return it
        const std::string modelName = getModelName(objectPath, modelLoadInfo);
        if (modelNameDataMap.contains(modelName)) {
          ModelData& modelData = modelNameDataMap[modelName];
          modelData.refCount++;
          return &modelData;
        }

        //Load the model data
        modelNameDataMap[modelName] = {};
        ModelData* modelData = &modelNameDataMap[modelName];
        modelData->refCount = 1;
        if (!internal::loadObject(objectPath, modelData, modelLoadInfo)) {
          modelNameDataMap.erase(modelName);
          return nullptr;
        }

        //Create buffers from loaded data
        createBuffers(modelData);

        return modelData;
      }

      ModelData* copyModelData(const std::string& modelName) {
        ModelData& modelData = modelNameDataMap[modelName];
        modelData.refCount++;
        return &modelData;
      }

      bool deleteModelData(const std::string& modelName) {
        //Check the model data is tracked
        if (!modelNameDataMap.contains(modelName)) {
          return false;
        }

        //Decrease the reference count of the model data
        ModelData* modelData = &modelNameDataMap[modelName];
        modelData->refCount--;

        //Delete the model data if this was the last reference
        if (modelData->refCount == 0) {
          //Reduce reference count on model data textures
          for (unsigned int i = 0; i < modelData->meshes.size(); i++) {
            if (modelData->textureIds[i].diffuseId != 0) {
              textures::internal::deleteTexture(modelData->textureIds[i].diffuseId);
            }

            if (modelData->textureIds[i].specularId != 0) {
              textures::internal::deleteTexture(modelData->textureIds[i].specularId);
            }
          }

          deleteBuffers(modelData);

          //Remove the map entry
          modelNameDataMap.erase(modelName);
        }

        return true;
      }
    }
  }
}
