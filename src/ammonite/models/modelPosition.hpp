#ifndef INTERNALMODELPOSITION
#define INTERNALMODELPOSITION

#include "modelTypes.hpp"

#include "../visibility.hpp"

namespace ammonite {
  namespace models {
    namespace AMMONITE_INTERNAL internal {
      //Calculate matrices from position, scale and rotation
      void calcModelMatrices(PositionData* positionData);
    }
  }
}

#endif
