#ifndef MODELTRACKER
#define MODELTRACKER

/* Internally exposed header:
 - Functions for direct access to model structures
*/

#include "modelTypes.hpp"
#include "../../constants.hpp"

namespace ammonite {
  namespace models {
    namespace internal {
      int getModelCount(AmmoniteEnum modelType);
      void getModels(AmmoniteEnum modelType, int modelCount, ModelInfo* modelArr[]);

      ModelInfo* getModelPtr(int modelId);
      bool* getModelsMovedPtr();
      void setLightEmitterId(int modelId, int lightEmitterId);
      int getLightEmitterId(int modelId);
    }
  }
}

#endif
