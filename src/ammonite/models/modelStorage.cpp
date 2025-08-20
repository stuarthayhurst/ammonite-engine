#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "models.hpp"

#include "../graphics/buffers.hpp"
#include "../graphics/textures.hpp"
#include "../utils/debug.hpp"
#include "../utils/id.hpp"

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
                              const ModelLoadInfo& modelLoadInfo,
                              AmmoniteId modelId) {
        //If the model has already been loaded, update counter, record ID and return it
        const std::string modelName = getModelName(objectPath, modelLoadInfo);
        if (modelNameDataMap.contains(modelName)) {
          ModelData& modelData = modelNameDataMap[modelName];
          modelData.refCount++;
          modelData.activeModelIds.insert(modelId);
          return &modelData;
        }

        //Load the model data
        std::vector<RawMeshData> rawMeshDataVec;
        modelNameDataMap[modelName] = {};
        ModelData* modelDataPtr = &modelNameDataMap[modelName];
        modelDataPtr->refCount = 1;
        if (!internal::loadObject(objectPath, modelDataPtr, &rawMeshDataVec, modelLoadInfo)) {
          modelNameDataMap.erase(modelName);
          return nullptr;
        }

        //Create buffers from loaded data
        graphics::internal::createModelBuffers(modelDataPtr, &rawMeshDataVec);

        modelDataPtr->activeModelIds.insert(modelId);
        return modelDataPtr;
      }

      ModelData* copyModelData(const std::string& modelName, AmmoniteId modelId) {
        ModelData& modelData = modelNameDataMap[modelName];
        modelData.refCount++;
        modelData.activeModelIds.insert(modelId);
        return &modelData;
      }

      ModelData* getModelData(const std::string& modelName) {
        return &modelNameDataMap[modelName];
      }

      bool deleteModelData(const std::string& modelName, AmmoniteId modelId) {
        //Check the model data is tracked
        if (!modelNameDataMap.contains(modelName)) {
          return false;
        }

        //Decrease the reference count of the model data
        ModelData* modelData = &modelNameDataMap[modelName];
        modelData->refCount--;
        if (modelData->activeModelIds.contains(modelId)) {
          modelData->activeModelIds.erase(modelId);
        } else {
          modelData->inactiveModelIds.erase(modelId);
        }

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

          graphics::internal::deleteModelBuffers(modelData);

          //Remove the map entry
          modelNameDataMap.erase(modelName);
          ammoniteInternalDebug << "Deleted storage for model data (" << modelName \
                                << ")" << std::endl;
        }

        return true;
      }

      void setModelInfoActive(AmmoniteId modelId, bool active) {
        ModelInfo* modelInfoPtr = getModelPtr(modelId);
        ModelData& modelData = modelNameDataMap[modelInfoPtr->modelName];
        if (active) {
          //Move the model ID to the active set
          modelData.inactiveModelIds.erase(modelId);
          modelData.activeModelIds.insert(modelId);
        } else {
          //Move the model ID to the inactive set
          modelData.activeModelIds.erase(modelId);
          modelData.inactiveModelIds.insert(modelId);
        }
      }

      //Return the number of unique model names
      unsigned int getModelNameCount() {
        return (unsigned int)modelNameDataMap.size();
      }

      //Return the number of ModelInfos for a modelName
      unsigned int getModelInfoCount(const std::string& modelName) {
        return (unsigned int)modelNameDataMap[modelName].activeModelIds.size();
      }

      //Fill an array with each unique model name
      void getModelNames(std::string* modelNameArray) {
        unsigned int i = 0;
        for (const auto& entry : modelNameDataMap) {
          modelNameArray[i++] = entry.first;
        }
      }

      //Fill an array with every ModelInfo* for a given modelName
      void getModelInfos(const std::string& modelName, ModelInfo* modelInfoArray[]) {
        const ModelData& modelData = modelNameDataMap[modelName];
        unsigned int i = 0;
        for (const AmmoniteId& modelId : modelData.activeModelIds) {
          modelInfoArray[i++] = getModelPtr(modelId);
        }
      }
    }
  }
}
