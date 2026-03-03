#ifndef MODELPLACEMENTMODE
#define MODELPLACEMENTMODE

#include <ammonite/ammonite.hpp>

namespace placement {
  void setPlacementCallbacks();
  void unsetPlacementCallbacks();

  void resetPlacementDistance();
  void updatePlacementPosition();
  void deletePlacedModels();

  void getPlacementColour(ammonite::Vec<float, 3>& dest);
  void setPlacementColour(const ammonite::Vec<float, 3>& colour);
}

#endif
