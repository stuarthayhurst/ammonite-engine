#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "models.hpp"

#include "../graphics/textures.hpp"

//TODO: Move to the graphics system
namespace {
  void createBuffers(ammonite::models::internal::ModelData* modelData,
                     std::vector<ammonite::models::internal::RawMeshData>* rawMeshDataVec) {
    //Generate buffers for every mesh
    for (const ammonite::models::internal::RawMeshData& rawMeshData : *rawMeshDataVec) {
      ammonite::models::internal::MeshInfoGroup& meshInfo = modelData->meshInfo.emplace_back();

      //Copy point count information
      meshInfo.vertexCount = rawMeshData.vertexCount;
      meshInfo.indexCount = rawMeshData.indexCount;

      //Create vertex and index buffers
      glCreateBuffers(1, &meshInfo.vertexBufferId);
      glCreateBuffers(1, &meshInfo.elementBufferId);

      //Fill interleaved vertex + normal + texture buffer and index buffer
      glNamedBufferData(meshInfo.vertexBufferId,
                        meshInfo.vertexCount * (long)sizeof(ammonite::models::internal::VertexData),
                        &rawMeshData.vertexData[0], GL_STATIC_DRAW);
      glNamedBufferData(meshInfo.elementBufferId,
                        meshInfo.indexCount * (long)sizeof(unsigned int),
                        &rawMeshData.indices[0], GL_STATIC_DRAW);

      //Destroy mesh data early
      delete [] rawMeshData.vertexData;
      delete [] rawMeshData.indices;

      //Create the vertex attribute buffer
      glCreateVertexArrays(1, &meshInfo.vertexArrayId);

      const GLuint vaoId = meshInfo.vertexArrayId;
      const GLuint vboId = meshInfo.vertexBufferId;
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
      glVertexArrayElementBuffer(vaoId, meshInfo.elementBufferId);
    }
  }

  void deleteBuffers(ammonite::models::internal::ModelData* modelData) {
    //Delete created buffers and the VAO
    for (const ammonite::models::internal::MeshInfoGroup& meshInfo : modelData->meshInfo) {
      glDeleteBuffers(1, &meshInfo.vertexBufferId);
      glDeleteBuffers(1, &meshInfo.elementBufferId);
      glDeleteVertexArrays(1, &meshInfo.vertexArrayId);
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
        std::vector<RawMeshData> rawMeshDataVec;
        modelNameDataMap[modelName] = {};
        ModelData* modelData = &modelNameDataMap[modelName];
        modelData->refCount = 1;
        if (!internal::loadObject(objectPath, modelData, &rawMeshDataVec, modelLoadInfo)) {
          modelNameDataMap.erase(modelName);
          return nullptr;
        }

        //Create buffers from loaded data
        createBuffers(modelData, &rawMeshDataVec);

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
          for (const TextureIdGroup& textureIdGroup : modelData->textureIds) {
            if (textureIdGroup.diffuseId != 0) {
              textures::internal::deleteTexture(textureIdGroup.diffuseId);
            }

            if (textureIdGroup.specularId != 0) {
              textures::internal::deleteTexture(textureIdGroup.specularId);
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
