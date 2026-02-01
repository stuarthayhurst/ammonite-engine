#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "models.hpp"

#include "../graphics/buffers.hpp"
#include "../graphics/textures.hpp"
#include "../utils/debug.hpp"
#include "../utils/id.hpp"

/*
 - Track model data (and transitively infos) against model keys
 - Manage model data storage
 - Supported queries:
   - Model key -> model data
   - Model key -> model infos
 - System-independent links:
   - Model info -> model ID
   - Model info -> model data
   - Model data -> model key
 - Use this for querying models by key
*/

namespace ammonite {
  namespace models {
    namespace internal {
      namespace {
        std::map<std::string, ModelData, std::less<>> modelKeyDataMap;
      }

      namespace {
        std::string calculateModelKey(const std::string& objectPath,
                                      const ModelLoadInfo& modelLoadInfo) {
          const unsigned char extraData = ((int)modelLoadInfo.flipTexCoords << 0) |
                                          ((int)modelLoadInfo.srgbTextures << 1);
          return objectPath + std::to_string(extraData);
        }
      }

      ModelData* addModelData(const std::string& objectPath,
                              const ModelLoadInfo& modelLoadInfo,
                              AmmoniteId modelId) {
        //If the model has already been loaded, update counter, record ID and return it
        const std::string modelKey = calculateModelKey(objectPath, modelLoadInfo);
        if (modelKeyDataMap.contains(modelKey)) {
          ModelData& modelData = modelKeyDataMap[modelKey];
          modelData.refCount++;
          modelData.activeModelIds.insert(modelId);
          return &modelData;
        }

        //Load the model data
        std::vector<RawMeshData> rawMeshDataVec;
        modelKeyDataMap[modelKey] = {};
        ModelData* const modelDataPtr = &modelKeyDataMap[modelKey];
        modelDataPtr->refCount = 1;
        modelDataPtr->modelKey = modelKey;
        if (!internal::loadObject(objectPath, modelDataPtr, &rawMeshDataVec, modelLoadInfo)) {
          modelKeyDataMap.erase(modelKey);
          return nullptr;
        }

        //Create buffers from loaded data
        graphics::internal::createModelBuffers(modelDataPtr, &rawMeshDataVec);

        modelDataPtr->activeModelIds.insert(modelId);
        return modelDataPtr;
      }

      ModelData* copyModelData(const std::string& modelKey, AmmoniteId modelId) {
        ModelData& modelData = modelKeyDataMap[modelKey];
        modelData.refCount++;
        modelData.activeModelIds.insert(modelId);
        return &modelData;
      }

      ModelData* getModelData(const std::string& modelKey) {
        return &modelKeyDataMap[modelKey];
      }

      ModelData* getModelData(std::string_view modelKey) {
        return &modelKeyDataMap.find(modelKey)->second;
      }

      bool deleteModelData(const std::string& modelKey, AmmoniteId modelId) {
        //Check the model data is tracked
        if (!modelKeyDataMap.contains(modelKey)) {
          return false;
        }

        //Decrease the reference count of the model data
        ModelData* const modelData = &modelKeyDataMap[modelKey];
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

          //Remove the map entry, potentially invalidating modelKey
          modelKeyDataMap.erase(modelKey);
          ammoniteInternalDebug << "Deleted storage for model data ('" << modelKey \
                                << "')" << std::endl;
        }

        return true;
      }

      void setModelInfoActive(AmmoniteId modelId, bool active) {
        ModelData* const modelDataPtr = getModelPtr(modelId)->modelData;
        if (active) {
          //Move the model ID to the active set
          modelDataPtr->inactiveModelIds.erase(modelId);
          modelDataPtr->activeModelIds.insert(modelId);
        } else {
          //Move the model ID to the inactive set
          modelDataPtr->activeModelIds.erase(modelId);
          modelDataPtr->inactiveModelIds.insert(modelId);
        }
      }

      //Return the number of unique model keys
      unsigned int getModelKeyCount() {
        return (unsigned int)modelKeyDataMap.size();
      }

      //Return the number of ModelInfos for a modelKey
      unsigned int getModelInfoCount(std::string_view modelKey) {
        return (unsigned int)modelKeyDataMap.find(modelKey)->second.activeModelIds.size();
      }

      //Fill an array with each unique model keys
      void getModelKeys(std::string_view modelKeyArray[]) {
        unsigned int i = 0;
        for (const auto& entry : modelKeyDataMap) {
          modelKeyArray[i++] = entry.first;
        }
      }

      //Fill an array with every ModelInfo* for a given modelKey
      void getModelInfos(std::string_view modelKey, ModelInfo* modelInfoArray[]) {
        const ModelData& modelData = modelKeyDataMap.find(modelKey)->second;
        unsigned int i = 0;
        for (const AmmoniteId& modelId : modelData.activeModelIds) {
          modelInfoArray[i++] = getModelPtr(modelId);
        }
      }
    }
  }
}
