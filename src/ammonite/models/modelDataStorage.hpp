#ifndef INTERNALMODELDATASTORAGE
#define INTERNALMODELDATASTORAGE

#include <string>
#include <string_view>

#include "modelTypes.hpp"

#include "../utils/id.hpp"
#include "../visibility.hpp"

namespace ammonite {
  namespace models {
    namespace AMMONITE_INTERNAL internal {
      //Model data storage management
      ModelData* addModelData(const ModelLoadInfo& modelLoadInfo,
                              AmmoniteId modelId);
      ModelData* copyModelData(const std::string& modelKey, AmmoniteId modelId);
      ModelData* getModelData(const std::string& modelKey);
      ModelData* getModelData(std::string_view modelKey);
      bool deleteModelData(const std::string& modelKey, AmmoniteId modelId);
      void setModelInfoActive(AmmoniteId modelId, bool active);

      //Model info retrieval, by model key
      unsigned int getModelKeyCount();
      unsigned int getModelInfoCount(std::string_view modelKey);
      void getModelKeys(std::string_view modelKeyArray[]);
      void getModelInfos(std::string_view modelKey, ModelInfo* modelInfoArray[]);
    }
  }
}

#endif
