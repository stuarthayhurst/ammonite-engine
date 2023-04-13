/* Internally exposed header:
 - Functions for direct access to model structures
*/

#ifndef MODELTRACKER
#define MODELTRACKER

#include "modelTypes.hpp"
#include "../../constants.hpp"

namespace ammonite {
  namespace models {
    int getModelCount(AmmoniteEnum modelType);
    void getModels(AmmoniteEnum modelType, int modelCount, ModelInfo* modelArr[]);

    ModelInfo* getModelPtr(int modelId);
    bool* getModelsMovedPtr();
    void setLightEmitting(int modelId, bool lightEmitting);
    bool getLightEmitting(int modelId);
  }
}

#endif
