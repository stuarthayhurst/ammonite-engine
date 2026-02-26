#include <cmath>

#include <ammonite/ammonite.hpp>

#include "torusGenerator.hpp"

namespace torus {
  namespace {
    //Return the previous point, within the same ring
    unsigned int previousPointWrapped(unsigned int index, unsigned int height) {
      return index - (index % height) + ((index - 1 + height) % height);
    }
  }

  /*
   - Return the maximum volume diameter for a given ring radius
     - ring radius + (volume diameter / 2) = 1
   - Keeps all vertices within [-1, 1]
  */
  float calculateMaxVolumeDiameter(float ringRadius) {
    return 2.0f * (1.0f - ringRadius);
  }

  /*
   - Return the maximum ring radius for a given volume diameter
     - ring radius + (volume diameter / 2) = 1
   - Keeps all vertices within [-1, 1]
  */
  float calculateMaxRingRadius(float volumeDiameter) {
    return 1.0f - (volumeDiameter / 2.0f);
  }

  //Return the number of vertices used in a mesh of these settings
  unsigned int getVertexCount(unsigned int widthNodes, unsigned int heightNodes) {
    return widthNodes * heightNodes;
  }

  //Return the number of indices used in a mesh of these settings
  unsigned int getIndexCount(unsigned int widthNodes, unsigned int heightNodes) {
    return getVertexCount(widthNodes, heightNodes) * 6;
  }

  /*
   - Generate indexed mesh data for a torus
     - The internal ring of the volume of the torus will has radius ringRadius
     - The volume ring will have diameter volumeDiameter
     - widthNodes determines how many vertical rings form the volume
     - heightNodes determines the number of vertices of these rings
   - meshVerticesPtr is a pointer to the location to store the mesh data's pointer
   - meshIndicesPtr is a pointer to the location to store the mesh indices' pointer
   - Both of these must be freed
  */
  void generateTorus(float ringRadius, float volumeDiameter,
                     unsigned int widthNodes, unsigned int heightNodes,
                     ammonite::models::AmmoniteVertex** meshVerticesPtr,
                     unsigned int** meshIndicesPtr) {
    const unsigned int vertexCount = widthNodes * heightNodes;
    ammonite::models::AmmoniteVertex* const meshVertices = new ammonite::models::AmmoniteVertex[vertexCount];
    unsigned int* const meshIndices = new unsigned int[vertexCount * 6ul];

    //Generate the mesh vertices and normals
    for (unsigned int widthNode = 0; widthNode < widthNodes; widthNode++) {
      const float ringRadians = ((float)widthNode / (float)widthNodes) * ammonite::twoPi<float>;

      //Calculate the origin for the current ring
      ammonite::Vec<float, 3> ringOrigin = {0};
      ammonite::scale(ammonite::calculateDirection(ringRadians, ringOrigin),
                      ringRadius);

      //Calculate the vertices and normals of the current ring
      for (unsigned int heightNode = 0; heightNode < heightNodes; heightNode++) {
        const float volumeRadians = ((float)heightNode / (float)heightNodes) * ammonite::twoPi<float>;
        const float localRadius = std::sin(volumeRadians) * volumeDiameter;

        //Calculate the vertex position
        const unsigned int skeletonIndex = (widthNode * heightNodes) + heightNode;
        ammonite::set(meshVertices[skeletonIndex].vertex,
          std::sin(ringRadians) * localRadius,
          std::cos(volumeRadians) / 2,
          std::cos(ringRadians) * localRadius
        );
        ammonite::scale(meshVertices[skeletonIndex].vertex, volumeDiameter);
        ammonite::add(meshVertices[skeletonIndex].vertex, ringOrigin);

        //Calculate the normal direction
        ammonite::normalise(ammonite::sub(meshVertices[skeletonIndex].vertex,
                                          ringOrigin,
                                          meshVertices[skeletonIndex].normal));

        //Fill the texture point with blank data
        ammonite::set(meshVertices[skeletonIndex].texturePoint, 0.0f);
      }
    }

    //Generate the indices for the mesh
    for (unsigned int i = 0; i < vertexCount; i++) {
      //Calculate the indices of the surrounding points for the triangle pair
      const unsigned int nextRingSamePoint = (i + heightNodes) % vertexCount;
      const unsigned int prevPoint = previousPointWrapped(i, heightNodes);
      const unsigned int nextRingPrevPoint = previousPointWrapped(nextRingSamePoint, heightNodes);

      //Save mesh indices to form the triangle pair
      meshIndices[(i * 6) + 0] = i;
      meshIndices[(i * 6) + 1] = nextRingSamePoint;
      meshIndices[(i * 6) + 2] = nextRingPrevPoint;
      meshIndices[(i * 6) + 3] = i;
      meshIndices[(i * 6) + 4] = nextRingPrevPoint;
      meshIndices[(i * 6) + 5] = prevPoint;
    }

    *meshVerticesPtr = meshVertices;
    *meshIndicesPtr = meshIndices;
  }
}
