#ifndef MODELTRACKER
#define MODELTRACKER

/* Internally exposed header:
 - Functions for direct access to model structures
*/

#include "modelTypes.hpp"
#include "../../enums.hpp"
#include "../../types.hpp"

namespace ammonite {
  namespace models {
    namespace internal {
      unsigned int getModelCount(AmmoniteEnum modelType);
      void getModels(AmmoniteEnum modelType, unsigned int modelCount, ModelInfo* modelArr[]);

      ModelInfo* getModelPtr(AmmoniteId modelId);
      bool* getModelsMovedPtr();
      void setLightEmitterId(AmmoniteId modelId, AmmoniteId lightEmitterId);
      AmmoniteId getLightEmitterId(AmmoniteId modelId);
    }
  }
}

#endif
