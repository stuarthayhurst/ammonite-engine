#ifndef TORUS
#define TORUS

#include <ammonite/ammonite.hpp>

namespace torus {
  unsigned int getVertexCount(unsigned int widthNodes, unsigned int heightNodes);
  unsigned int getIndexCount(unsigned int widthNodes, unsigned int heightNodes);

  float calculateMaxVolumeDiameter(float ringRadius);
  float calculateMaxRingRadius(float volumeDiameter);

  void generateTorus(float ringRadius, float volumeDiameter,
                     unsigned int widthNodes, unsigned int heightNodes,
                     ammonite::models::AmmoniteVertex** meshVerticesPtr,
                     unsigned int** meshIndicesPtr);
}

#endif
