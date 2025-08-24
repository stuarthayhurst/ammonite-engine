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
    void dumpModelStorageDebug() {
#ifdef AMMONITE_DEBUG
      //Create storage for model names, query the names
      const unsigned int modelNameCount = internal::getModelNameCount();
      std::string_view* modelNames = new std::string_view[modelNameCount];
      internal::getModelNames(modelNames);

      for (unsigned int modelNameIndex = 0; modelNameIndex < modelNameCount; modelNameIndex++) {
        //Create storage for ModelInfo pointers, query them
        const std::string_view& modelName = modelNames[modelNameIndex];
        const unsigned int modelInfoCount = internal::getModelInfoCount(modelName);
        internal::ModelInfo** modelInfoPtrs = new internal::ModelInfo*[modelInfoCount];
        internal::getModelInfos(modelName, modelInfoPtrs);

        //Debug print the model name
        ammoniteInternalDebug << "Model name " << modelNameIndex << ": '" \
                              << modelName << "'" << std::endl;
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

      ammoniteInternalDebug << std::endl;
      delete [] modelNames;
#endif
    }
  }
}
