#include <cstring>
#include <iostream>
#include <queue>
#include <string>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/types.h>

#include "models.hpp"

#include "../graphics/textures.hpp"
#include "../utils/logging.hpp"

namespace ammonite {
  namespace models {
    namespace internal {
      namespace {
        bool processMesh(aiMesh* meshPtr, const aiScene* scenePtr,
                                models::internal::ModelData* modelObjectData,
                                const ModelLoadInfo& modelLoadInfo) {
          std::vector<models::internal::TextureIdGroup>* textureIds = &modelObjectData->textureIds;

          //Add a new empty mesh to the mesh vector
          models::internal::MeshData* newMesh = &modelObjectData->meshes.emplace_back();
          newMesh->vertexCount = meshPtr->mNumVertices;
          newMesh->meshData = new VertexData[meshPtr->mNumVertices];

          //Fill the mesh with vertex data
          bool hasWarnedMesh = false;
          for (unsigned int i = 0; i < meshPtr->mNumVertices; i++) {
            newMesh->meshData[i].vertex.x = meshPtr->mVertices[i].x;
            newMesh->meshData[i].vertex.y = meshPtr->mVertices[i].y;
            newMesh->meshData[i].vertex.z = meshPtr->mVertices[i].z;

            newMesh->meshData[i].vertex = glm::vec3(meshPtr->mVertices[i].x,
              meshPtr->mVertices[i].y, meshPtr->mVertices[i].z);

            newMesh->meshData[i].normal = glm::vec3(meshPtr->mNormals[i].x,
              meshPtr->mNormals[i].y, meshPtr->mNormals[i].z);

            if (meshPtr->mTextureCoords[0] != nullptr) {
              newMesh->meshData[i].texturePoint = glm::vec2(meshPtr->mTextureCoords[0][i].x,
                meshPtr->mTextureCoords[0][i].y);
            } else {
              if (!hasWarnedMesh) {
                ammonite::utils::warning << "Missing texture coord data for mesh" << std::endl;
                hasWarnedMesh = true;
              }
              newMesh->meshData[i].texturePoint = glm::vec2(0.0f);
            }
          }

          //Count mesh indices
          for (unsigned int i = 0; i < meshPtr->mNumFaces; i++) {
            newMesh->indexCount += meshPtr->mFaces[i].mNumIndices;
          }

          //Allocate and fill mesh indices
          newMesh->indices = new unsigned int[newMesh->indexCount];
          unsigned int index = 0;
          for (unsigned int i = 0; i < meshPtr->mNumFaces; i++) {
            aiFace face = meshPtr->mFaces[i];
            if (&face.mIndices[0] != nullptr) {
              std::memcpy(&newMesh->indices[index], &face.mIndices[0],
                          face.mNumIndices * sizeof(unsigned int));
            }
            index += face.mNumIndices;
          }

          //Fetch material for the mesh
          aiMaterial *material = scenePtr->mMaterials[meshPtr->mMaterialIndex];
          aiString localTexturePath;
          std::string fullTexturePath;
          models::internal::TextureIdGroup textureIdGroup;

          if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            material->GetTexture(aiTextureType_DIFFUSE, 0, &localTexturePath);
            fullTexturePath = modelLoadInfo.modelDirectory + '/' + localTexturePath.C_Str();

            GLuint textureId = ammonite::textures::internal::loadTexture(fullTexturePath,
              false, modelLoadInfo.srgbTextures);
            if (textureId == 0) {
              return false;
            }

            textureIdGroup.diffuseId = textureId;
          }

          if (material->GetTextureCount(aiTextureType_SPECULAR) > 0) {
            material->GetTexture(aiTextureType_SPECULAR, 0, &localTexturePath);
            fullTexturePath = modelLoadInfo.modelDirectory + '/' + localTexturePath.C_Str();

            GLuint textureId = ammonite::textures::internal::loadTexture(fullTexturePath,
              false, modelLoadInfo.srgbTextures);
            if (textureId == 0) {
              return false;
            }

            textureIdGroup.specularId = textureId;
          }

          //Save texture IDs
          textureIds->push_back(textureIdGroup);
          return true;
        }

        bool processNodes(const aiScene* scenePtr,
                                 models::internal::ModelData* modelObjectData,
                                 const ModelLoadInfo& modelLoadInfo) {
          std::queue<aiNode*> nodePtrQueue;
          nodePtrQueue.push(scenePtr->mRootNode);

          //Process root node, then process any more connected to it
          bool passed = true;
          while (!nodePtrQueue.empty()) {
            aiNode* nodePtr = nodePtrQueue.front();
            nodePtrQueue.pop();

            //Process meshes
            for (unsigned int i = 0; i < nodePtr->mNumMeshes; i++) {
              passed &= processMesh(scenePtr->mMeshes[nodePtr->mMeshes[i]], scenePtr,
                                    modelObjectData, modelLoadInfo);
            }

            //Add connected nodes to queue
            for (unsigned int i = 0; i < nodePtr->mNumChildren; i++) {
              nodePtrQueue.push(nodePtr->mChildren[i]);
            }
          }

          return passed;
        }
      }

      /*
       - Load an object from objectPath, using the settings from modelLoadInfo
       - Store the model's data using the modelObjectData pointer
      */
      bool loadObject(const std::string& objectPath, models::internal::ModelData* modelObjectData,
                      const ModelLoadInfo& modelLoadInfo) {
        //Generate post-processing flags
        auto aiProcessFlags = aiProcess_Triangulate |
                              aiProcess_GenNormals |
                              aiProcess_GenUVCoords |
                              aiProcess_RemoveRedundantMaterials |
                              aiProcess_OptimizeMeshes |
                              aiProcess_JoinIdenticalVertices |
                              aiProcess_PreTransformVertices;

        //Flip texture coords, if requested
        if (modelLoadInfo.flipTexCoords) {
          aiProcessFlags = aiProcessFlags | aiProcess_FlipUVs;
        }

        Assimp::Importer importer;
        const aiScene* scenePtr = importer.ReadFile(objectPath.c_str(), aiProcessFlags);

        //Check model loaded correctly
        if (scenePtr == nullptr ||
            (scenePtr->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 ||
            scenePtr->mRootNode == nullptr) {
          ammonite::utils::warning << importer.GetErrorString() << std::endl;
          return false;
        }

        //Recursively process nodes
        return processNodes(scenePtr, modelObjectData, modelLoadInfo);
      }
    }
  }
}
