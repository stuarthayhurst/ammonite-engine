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
#include "../maths/vector.hpp"
#include "../utils/debug.hpp"
#include "../utils/logging.hpp"

namespace ammonite {
  namespace models {
    namespace internal {
      namespace {
        bool processMesh(aiMesh* meshPtr, const aiScene* scenePtr,
                         ModelData* modelData, std::vector<RawMeshData>* rawMeshDataVec,
                         const ModelLoadInfo& modelLoadInfo) {
          std::vector<models::internal::TextureIdGroup>* textureIds = &modelData->textureIds;

          //Add a new empty mesh to the mesh vector
          models::internal::RawMeshData* const newMesh = &rawMeshDataVec->emplace_back();
          newMesh->vertexCount = meshPtr->mNumVertices;
          newMesh->vertexData = new VertexData[meshPtr->mNumVertices];

          //Fill the mesh with vertex data
          bool hasWarnedMesh = false;
          for (unsigned int i = 0; i < meshPtr->mNumVertices; i++) {
            ammonite::set(newMesh->vertexData[i].vertex, meshPtr->mVertices[i].x,
                          meshPtr->mVertices[i].y, meshPtr->mVertices[i].z);

            ammonite::set(newMesh->vertexData[i].normal, meshPtr->mNormals[i].x,
                          meshPtr->mNormals[i].y, meshPtr->mNormals[i].z);

            if (meshPtr->mTextureCoords[0] != nullptr) {
              ammonite::set(newMesh->vertexData[i].texturePoint,
                            meshPtr->mTextureCoords[0][i].x,
                            meshPtr->mTextureCoords[0][i].y);
            } else {
              if (!hasWarnedMesh) {
                ammonite::utils::warning << "Missing texture coordinate data for mesh" << std::endl;
                hasWarnedMesh = true;
              }
              ammonite::set(newMesh->vertexData[i].texturePoint, 0.0f);
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
            const aiFace face = meshPtr->mFaces[i];
            if (&face.mIndices[0] != nullptr) {
              std::memcpy(&newMesh->indices[index], &face.mIndices[0],
                          face.mNumIndices * sizeof(unsigned int));
            }
            index += face.mNumIndices;
          }

          //Fetch material for the mesh
          const aiMaterial* const material = scenePtr->mMaterials[meshPtr->mMaterialIndex];
          aiString localTexturePath;
          std::string fullTexturePath;
          models::internal::TextureIdGroup textureIdGroup;

          if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            material->GetTexture(aiTextureType_DIFFUSE, 0, &localTexturePath);
            fullTexturePath = modelLoadInfo.modelDirectory + '/' + localTexturePath.C_Str();

            const GLuint textureId = ammonite::textures::internal::loadTexture(
              fullTexturePath, false, modelLoadInfo.srgbTextures);
            if (textureId == 0) {
              return false;
            }

            textureIdGroup.diffuseId = textureId;
          } else {
            ammoniteInternalDebug << "No diffuse texture supplied on mesh of model '" << modelData->modelName << "', skipping" << std::endl;
          }

          if (material->GetTextureCount(aiTextureType_SPECULAR) > 0) {
            material->GetTexture(aiTextureType_SPECULAR, 0, &localTexturePath);
            fullTexturePath = modelLoadInfo.modelDirectory + '/' + localTexturePath.C_Str();

            const GLuint textureId = ammonite::textures::internal::loadTexture(
              fullTexturePath, false, modelLoadInfo.srgbTextures);
            if (textureId == 0) {
              return false;
            }

            textureIdGroup.specularId = textureId;
          }

          //Save texture IDs
          textureIds->push_back(textureIdGroup);
          return true;
        }

        bool processNodes(const aiScene* scenePtr, ModelData* modelData,
                          std::vector<RawMeshData>* rawMeshDataVec,
                          const ModelLoadInfo& modelLoadInfo) {
          std::queue<aiNode*> nodePtrQueue;
          nodePtrQueue.push(scenePtr->mRootNode);

          //Process root node, then process any more connected to it
          bool passed = true;
          while (!nodePtrQueue.empty()) {
            const aiNode* const nodePtr = nodePtrQueue.front();
            nodePtrQueue.pop();

            //Process meshes
            for (unsigned int i = 0; i < nodePtr->mNumMeshes; i++) {
              passed &= processMesh(scenePtr->mMeshes[nodePtr->mMeshes[i]], scenePtr,
                                    modelData, rawMeshDataVec, modelLoadInfo);
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
       - Store the model's mesh data to rawMeshDataVec and texture data to modelData
      */
      bool loadObject(const std::string& objectPath, ModelData* modelData,
                      std::vector<RawMeshData>* rawMeshDataVec,
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
        const aiScene* const scenePtr = importer.ReadFile(objectPath.c_str(), aiProcessFlags);

        //Check model loaded correctly
        if (scenePtr == nullptr ||
            (scenePtr->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 ||
            scenePtr->mRootNode == nullptr) {
          ammonite::utils::warning << importer.GetErrorString() << std::endl;
          return false;
        }

        //Recursively process nodes
        return processNodes(scenePtr, modelData, rawMeshDataVec, modelLoadInfo);
      }
    }
  }
}
