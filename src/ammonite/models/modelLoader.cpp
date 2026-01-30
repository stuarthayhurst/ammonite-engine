#include <cstring>
#include <iostream>
#include <list>
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
#include "../utils/thread.hpp"

/*
 - Process a model into internal structures
 - Load the model from a path and some settings
 - Fills in the mesh and texture parts of a ModelData
*/

namespace ammonite {
  namespace models {
    namespace internal {
      namespace {
        struct MatKey {
          const char* key;
          unsigned int type;
          unsigned int index;
        };

        //Data required by the worker thread to load the texture
        struct TextureThreadData {
          std::string texturePath;
          bool flipTextures;
          bool srgbTextures;
          textures::internal::TextureData textureData;
          bool loadedTexture;
        };

        //Hold data to load a texture and the sync for its worker
        struct TextureLoadData {
          TextureThreadData threadData;
          GLuint textureId;
          AmmoniteGroup sync{0};
        };

        //Store the outstanding texture loads
        std::list<TextureLoadData> textureQueue;
      }

      namespace {
        void textureLoadWorker(void* userPtr) {
          TextureThreadData* const threadData = (TextureThreadData*)userPtr;

          const bool prepared = textures::internal::prepareTextureData(
            threadData->texturePath, threadData->flipTextures,
            threadData->srgbTextures, &threadData->textureData);

          threadData->loadedTexture = prepared;
        }
      }

      namespace {
        bool materialHasTexture(const aiMaterial* materialPtr, aiTextureType textureType) {
          return (materialPtr->GetTextureCount(textureType) > 0);
        }

        GLuint processTexture(const aiMaterial* materialPtr, aiTextureType textureType,
                              const ModelLoadInfo& modelLoadInfo, const std::string& modelName) {
          //Bail if we don't have any of this type
          if (!materialHasTexture(materialPtr, textureType)) {
            ammoniteInternalDebug << "Attempted to load texture on '" << modelName \
                                  << "', but none of this type exist" << std::endl;
            return 0;
          }

          aiString localTexturePath;
          materialPtr->GetTexture(textureType, 0, &localTexturePath);
          const std::string fullTexturePath = modelLoadInfo.modelDirectory + '/' + localTexturePath.C_Str();

          //Calculate the texture's key
          std::string textureKey;
          textures::internal::calculateTextureKey(fullTexturePath, false, modelLoadInfo.srgbTextures, &textureKey);

          //Use texture cache, if already loaded / reserved
          if (textures::internal::checkTextureKey(textureKey)) {
            return textures::internal::acquireTextureKeyId(textureKey);
          }

          //Reserve the texture key before loading
          const GLuint textureId = textures::internal::reserveTextureKey(textureKey);
          if (textureId == 0) {
            ammonite::utils::warning << "Failed to reserve texture" << std::endl;
            return 0;
          }

          //Prepare data for the worker thread
          TextureLoadData& textureLoadData = textureQueue.emplace_back();
          textureLoadData.threadData.texturePath = fullTexturePath;
          textureLoadData.threadData.flipTextures = false;
          textureLoadData.threadData.srgbTextures = modelLoadInfo.srgbTextures;
          textureLoadData.threadData.loadedTexture = false;

          //Prepare date for texture upload after sync
          textureLoadData.textureId = textureId;

          //Submit the texture load to the thread pool
          ammonite::utils::thread::submitWork(textureLoadWorker,
            &textureLoadData.threadData, &textureLoadData.sync);

          return textureId;
        }

        bool materialHasColour(const aiMaterial* materialPtr, const MatKey& colourKey) {
          aiColor3D aiColour(0.0f, 0.0f, 0.0f);
          return (materialPtr->Get(colourKey.key, colourKey.type, colourKey.index, aiColour) == AI_SUCCESS);
        }

        GLuint processColour(const aiMaterial* materialPtr,
                             const MatKey& colourKey, const std::string& modelName) {
          //Bail if we don't have any of this type
          if (!materialHasColour(materialPtr, colourKey)) {
            ammoniteInternalDebug << "Attempted to load colour on '" << modelName \
                                  << "', but none of this type exist" << std::endl;
            return 0;
          }

          //Fetch the colour
          aiColor3D aiColour(0.0f, 0.0f, 0.0f);
          materialPtr->Get(colourKey.key, colourKey.type, colourKey.index, aiColour);

          const ammonite::Vec<float, 3> colour = {aiColour.r, aiColour.g, aiColour.b};
          return ammonite::textures::internal::loadSolidTexture(colour);
        }

        //Load all components of a material into a TextureIdGroup
        TextureIdGroup processMaterial(const aiMaterial* materialPtr,
                                       const ModelLoadInfo& modelLoadInfo,
                                       const std::string& modelName) {
          TextureIdGroup textureGroup = {0};

          //Array of info required to fill the texture group by texture type
          const unsigned int textureTypeCount = 2;
          const struct TextureLoadInfo {
            aiTextureType textureType;
            MatKey colourKey;
            GLuint* textureIdPtr;
            bool isRequired;
          } textureLoadInfo[textureTypeCount] = {
            {aiTextureType_DIFFUSE, {AI_MATKEY_COLOR_DIFFUSE}, &textureGroup.diffuseId, true},
            {aiTextureType_SPECULAR, {AI_MATKEY_COLOR_SPECULAR}, &textureGroup.specularId, false}
          };

          //Load each texture type of the material, according to its parameters
          bool missing = false;
          for (const TextureLoadInfo& loadInfo : textureLoadInfo) {
            //Load the material texture type as a texture or a colour
            if (materialHasTexture(materialPtr, loadInfo.textureType)) {
              //Attempt to load the material as a texture
              *loadInfo.textureIdPtr = processTexture(materialPtr, loadInfo.textureType,
                                                      modelLoadInfo, modelName);
            } else if (materialHasColour(materialPtr, loadInfo.colourKey)) {
              //Attempt to load the material as a colour
              *loadInfo.textureIdPtr = processColour(materialPtr, loadInfo.colourKey,
                                                     modelName);
            } else {
              missing = true;
            }

            //Debug warnings for missing or failed required material components
            if (loadInfo.isRequired) {
              if (missing) {
                //Debug warn if the material was required and missing
                ammoniteInternalDebug << "Mandatory texture / colour not supplied for model '" \
                                      << modelName << "', skipping" << std::endl;
              } else if (*loadInfo.textureIdPtr == 0) {
                ammoniteInternalDebug << "Mandatory texture / colour couldn't be loaded for model '" \
                                      << modelName << "', skipping" << std::endl;
              }
            }
          }

          return textureGroup;
        }

        bool processMesh(aiMesh* meshPtr, const aiScene* scenePtr,
                         ModelData* modelData, std::vector<RawMeshData>* rawMeshDataVec,
                         const ModelLoadInfo& modelLoadInfo) {
          std::vector<models::internal::TextureIdGroup>* textureIds = &modelData->textureIds;

          //Process material into a texture ID group as early as possible
          const aiMaterial* const materialPtr = scenePtr->mMaterials[meshPtr->mMaterialIndex];
          textureIds->push_back(processMaterial(materialPtr, modelLoadInfo, modelData->modelName));

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
        const bool loadedNodes = processNodes(scenePtr, modelData,
                                              rawMeshDataVec, modelLoadInfo);

        //Wait for the texture loads to complete, and upload their data
        bool uploadedTextures = true;
        for (TextureLoadData& textureLoadData : textureQueue) {
          ammonite::utils::thread::waitGroupComplete(&textureLoadData.sync, 1);

          //Don't attempt to upload failed textures
          if (!textureLoadData.threadData.loadedTexture) {
            uploadedTextures = false;
            continue;
          }

          //Upload the loaded texture
          uploadedTextures &= textures::internal::uploadTextureData(
            textureLoadData.textureId, textureLoadData.threadData.textureData);
        }

        //Clear the texture queue
        textureQueue.clear();

        return loadedNodes && uploadedTextures;
      }
    }
  }
}
