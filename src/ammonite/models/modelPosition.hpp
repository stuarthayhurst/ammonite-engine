#ifndef INTERNALMODELPOSITION
#define INTERNALMODELPOSITION

#include "modelTypes.hpp"

#include "../visibility.hpp"

namespace AMMONITE_INTERNAL ammonite {
  namespace models {
    namespace internal {
      //Calculate matrices from position, scale and rotation
      void calcModelMatrices(PositionData* positionData);
    }
  }
}

#endif
