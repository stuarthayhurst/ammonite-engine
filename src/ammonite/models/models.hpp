#ifndef INTERNALMODELS
#define INTERNALMODELS

//Include other model components
#include "loaders/modelLoader.hpp" // IWYU pragma: export
#include "modelPosition.hpp" // IWYU pragma: export
#include "modelDataStorage.hpp" // IWYU pragma: export
#include "modelTypes.hpp" // IWYU pragma: export

#include "../utils/id.hpp"
#include "../visibility.hpp"

//Include public interface
#include "../../include/ammonite/models/models.hpp" // IWYU pragma: export

namespace AMMONITE_INTERNAL ammonite {
  namespace models {
    namespace internal {
      //Model info retrieval
      unsigned int getModelCount(ModelTypeEnum modelType);
      void getModels(ModelTypeEnum modelType, unsigned int modelCount,
                     ModelInfo* modelInfoArray[]);
      ModelInfo* getModelPtr(AmmoniteId modelId);
      bool* getModelsMovedPtr();

      void setLightEmitterId(AmmoniteId modelId, AmmoniteId lightEmitterId);
      AmmoniteId getLightEmitterId(AmmoniteId modelId);
    }
  }
}

#endif
