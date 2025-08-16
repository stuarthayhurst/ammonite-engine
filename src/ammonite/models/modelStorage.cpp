#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "models.hpp"

#include "../graphics/buffers.hpp"
#include "../graphics/textures.hpp"
#include "../utils/debug.hpp"

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
        graphics::internal::createModelBuffers(modelData, &rawMeshDataVec);

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

          graphics::internal::deleteModelBuffers(modelData);

          //Remove the map entry
          modelNameDataMap.erase(modelName);
          ammoniteInternalDebug << "Deleted storage for model data (" << modelName \
                                << ")" << std::endl;
        }

        return true;
      }
    }
  }
}
