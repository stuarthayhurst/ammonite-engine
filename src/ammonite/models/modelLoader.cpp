#include <iostream>
#include <vector>
#include <queue>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "internal/modelTypes.hpp"
#include "internal/modelLoader.hpp"

#include "../graphics/internal/internalTextures.hpp"
#include "../utils/logging.hpp"

namespace ammonite {
  namespace models {
    namespace internal {
      namespace {
        static void processMesh(aiMesh* meshPtr, const aiScene* scenePtr,
                                models::internal::ModelData* modelObjectData,
                                ModelLoadInfo modelLoadInfo,
                                bool* externalSuccess) {
          std::vector<models::internal::MeshData>* meshes = &modelObjectData->meshes;
          std::vector<models::internal::TextureIdGroup>* textureIds = &modelObjectData->textureIds;

          //Add a new empty mesh to the mesh vector
          meshes->emplace_back();
          models::internal::MeshData* newMesh = &meshes->back();

          //Fill the mesh with vertex data
          bool hasWarnedMesh = false;
          for (unsigned int i = 0; i < meshPtr->mNumVertices; i++) {
            models::internal::VertexData vertexData;

            vertexData.vertex.x = meshPtr->mVertices[i].x;
            vertexData.vertex.y = meshPtr->mVertices[i].y;
            vertexData.vertex.z = meshPtr->mVertices[i].z;

            vertexData.normal.x = meshPtr->mNormals[i].x;
            vertexData.normal.y = meshPtr->mNormals[i].y;
            vertexData.normal.z = meshPtr->mNormals[i].z;

            if (meshPtr->mTextureCoords[0]) {
              vertexData.texturePoint.x = meshPtr->mTextureCoords[0][i].x;
              vertexData.texturePoint.y = meshPtr->mTextureCoords[0][i].y;
            } else {
              if (!hasWarnedMesh) {
                ammonite::utils::warning << "Missing texture coord data for mesh" << std::endl;
                hasWarnedMesh = true;
              }
              vertexData.texturePoint = glm::vec2(0.0f);
            }

            newMesh->meshData.push_back(vertexData);
          }

          //Fill mesh indices
          for (unsigned int i = 0; i < meshPtr->mNumFaces; i++) {
            aiFace face = meshPtr->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
              newMesh->indices.push_back(face.mIndices[j]);
            }
          }
          newMesh->vertexCount = newMesh->indices.size();

          //Fetch material for the mesh
          aiMaterial *material = scenePtr->mMaterials[meshPtr->mMaterialIndex];
          aiString localTexturePath;
          std::string fullTexturePath;
          models::internal::TextureIdGroup textureIdGroup;

          if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            material->GetTexture(aiTextureType_DIFFUSE, 0, &localTexturePath);
            fullTexturePath = modelLoadInfo.modelDirectory + '/' + localTexturePath.C_Str();

            bool createdTextureSuccess = true;
            int textureId = ammonite::textures::internal::loadTexture(fullTexturePath.c_str(),
                                                                      modelLoadInfo.srgbTextures,
                                                                      &createdTextureSuccess);
            if (!createdTextureSuccess) {
              *externalSuccess = false;
              return;
            }

            textureIdGroup.diffuseId = textureId;
          }

          if (material->GetTextureCount(aiTextureType_SPECULAR) > 0) {
            material->GetTexture(aiTextureType_SPECULAR, 0, &localTexturePath);
            fullTexturePath = modelLoadInfo.modelDirectory + '/' + localTexturePath.C_Str();

            bool createdTextureSuccess = true;
            int textureId = ammonite::textures::internal::loadTexture(fullTexturePath.c_str(),
                                                                      modelLoadInfo.srgbTextures,
                                                                      &createdTextureSuccess);
            if (!createdTextureSuccess) {
              *externalSuccess = false;
              return;
            }

            textureIdGroup.specularId = textureId;
          }

          //Save texture IDs
          textureIds->push_back(textureIdGroup);
        }

        static void processNodes(const aiScene* scenePtr,
                               models::internal::ModelData* modelObjectData,
                               ModelLoadInfo modelLoadInfo, bool* externalSuccess) {
          std::queue<aiNode*> nodePtrQueue;
          nodePtrQueue.push(scenePtr->mRootNode);

          //Process root node, then process any more connected to it
          while (!nodePtrQueue.empty()) {
            aiNode* nodePtr = nodePtrQueue.front();
            nodePtrQueue.pop();

            //Process meshes
            for (unsigned int i = 0; i < nodePtr->mNumMeshes; i++) {
              processMesh(scenePtr->mMeshes[nodePtr->mMeshes[i]], scenePtr, modelObjectData,
                          modelLoadInfo, externalSuccess);
            }

            //Add connected nodes to queue
            for (unsigned int i = 0; i < nodePtr->mNumChildren; i++) {
              nodePtrQueue.push(nodePtr->mChildren[i]);
            }
          }
        }
      }

      void loadObject(const char* objectPath, models::internal::ModelData* modelObjectData,
                      ModelLoadInfo modelLoadInfo, bool* externalSuccess) {
        //Generate postprocessing flags
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
        const aiScene* scenePtr = importer.ReadFile(objectPath, aiProcessFlags);

        //Check model loaded correctly
        if (!scenePtr || scenePtr->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scenePtr->mRootNode) {
          ammonite::utils::warning << importer.GetErrorString() << std::endl;
          *externalSuccess = false;
          return;
        }

        //Recursively process nodes
        processNodes(scenePtr, modelObjectData, modelLoadInfo, externalSuccess);
      }
    }
  }
}
