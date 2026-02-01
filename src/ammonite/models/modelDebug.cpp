#ifdef AMMONITE_DEBUG
#include <iostream>
#include <string_view>
#endif

#include "models.hpp"

#ifdef AMMONITE_DEBUG
#include "../utils/debug.hpp"
#endif

namespace ammonite {
  namespace models {
    bool dumpModelStorageDebug() {
#ifdef AMMONITE_DEBUG
      //Create storage for model keys, query the names
      const unsigned int modelKeyCount = internal::getModelKeyCount();
      std::string_view* const modelKeys = new std::string_view[modelKeyCount];
      internal::getModelKeys(modelKeys);

      for (unsigned int modelKeyIndex = 0; modelKeyIndex < modelKeyCount; modelKeyIndex++) {
        //Create storage for ModelInfo pointers, query them
        const std::string_view& modelKey = modelKeys[modelKeyIndex];
        const unsigned int modelInfoCount = internal::getModelInfoCount(modelKey);
        internal::ModelInfo** const modelInfoPtrs = new internal::ModelInfo*[modelInfoCount];
        internal::getModelInfos(modelKey, modelInfoPtrs);

        //Debug print the model key
        ammoniteInternalDebug << "Model key " << modelKeyIndex << ": '" \
                              << modelKey << "'" << std::endl;
        ammoniteInternalDebug << "  Active model IDs (" << modelInfoCount << "): ";

        //Debug print each ID using the name
        for (unsigned int i = 0; i < modelInfoCount; i++) {
          if (i != 0) {
            ammoniteInternalDebug << ", ";
          }

          ammoniteInternalDebug << modelInfoPtrs[i]->modelId;
          if (modelInfoPtrs[i]->modelType == AMMONITE_LIGHT_EMITTER) {
            ammoniteInternalDebug << "*";
          }
        }

        ammoniteInternalDebug << std::endl;
        delete [] modelInfoPtrs;
      }

      ammoniteInternalDebug.printEmptyLine();
      delete [] modelKeys;
      return true;
#else
      return false;
#endif
    }
  }
}
