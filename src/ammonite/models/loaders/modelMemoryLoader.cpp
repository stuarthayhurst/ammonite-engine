#include <cstring>
#include <map>
#include <vector>

#include "../models.hpp"

#include "../../graphics/textures.hpp"

/*
 - Process a (memory-based) model into internal structures
 - Load the model from memory and some settings
   - Memory must provide meshes and materials
   - Meshes will be indexed if necessary
 - Fills in the mesh and texture parts of a ModelData
*/

namespace ammonite {
  namespace models {
    namespace internal {
      namespace {
        //Allow AmmoniteVertex as a map key, accounting for padding
        //NOLINTBEGIN(bugprone-suspicious-memory-comparison)
        struct VertexCompare {
          bool operator()(const AmmoniteVertex& left,
                          const AmmoniteVertex& right) const {
            if constexpr (sizeof(AmmoniteVertex) == (sizeof(left.vertex) + \
                          sizeof(left.normal) + sizeof(left.texturePoint))) {
              //No padding, just compare the memory
              return std::memcmp(&left, &right, sizeof(AmmoniteVertex)) < 0;
            } else {
              //Padding, compare relevant sections
              int res = std::memcmp(&left.vertex, &right.vertex, sizeof(left.vertex));
              if (res != 0) {
                return res < 0;
              }

              res = std::memcmp(&left.normal, &right.normal, sizeof(left.normal));
              if (res != 0) {
                return res < 0;
              }

              res = std::memcmp(&left.texturePoint, &right.texturePoint, sizeof(left.texturePoint));
              return res < 0;
            }
          }
        };
        //NOLINTEND(bugprone-suspicious-memory-comparison)

        //Copy the mesh data into the raw mesh data vector
        void applyMesh(std::vector<RawMeshData>* rawMeshDataVec, unsigned int vertexCount, const AmmoniteVertex* vertexData, unsigned int indexCount, const unsigned int* indices) {
          //Copy vertices
          RawMeshData& newMesh = rawMeshDataVec->emplace_back();
          newMesh.vertexCount = vertexCount;
          newMesh.vertexData = new AmmoniteVertex[vertexCount];
          std::memcpy(newMesh.vertexData, vertexData, sizeof(AmmoniteVertex) * vertexCount);

          //Copy indices
          newMesh.indexCount = indexCount;
          newMesh.indices = new unsigned int[indexCount];
          std::memcpy(newMesh.indices, indices, sizeof(unsigned int) * indexCount);
        }

        //Index then copy mesh data into a vector of RawMeshData
        void indexMeshes(const AmmoniteVertex** meshArray, unsigned int meshCount,
                         const unsigned int* vertexCounts,
                         std::vector<RawMeshData>* rawMeshDataVec) {
          //Index and upload each mesh
          for (unsigned int meshIndex = 0; meshIndex < meshCount; meshIndex++) {
            const unsigned int vertexCount = vertexCounts[meshIndex];
            const AmmoniteVertex* const mesh = meshArray[meshIndex];

            //Store the generated indices and unique vertices
            std::vector<AmmoniteVertex> uniqueVertices;
            std::vector<unsigned int> indices;
            indices.reserve(vertexCount);

            //Generate indices and unique vertices for the mesh
            std::map<AmmoniteVertex, unsigned int, VertexCompare> vertexIndexMap;
            unsigned int nextIndex = 0;
            for (unsigned int i = 0; i < vertexCount; i++) {
              const AmmoniteVertex& vertexData = mesh[i];

              //Fetch or generate an index for the vertex
              unsigned int index = 0;
              if (!vertexIndexMap.contains(vertexData)) {
                //Generate a new index and save the vertex data
                index = nextIndex++;
                vertexIndexMap[vertexData] = index;
                uniqueVertices.push_back(vertexData);
              } else {
                //Fetch the index for the vertex
                index = vertexIndexMap[vertexData];
              }

              //Save the index in order
              indices.push_back(index);
            }

            //Copy the vertices and indices to the data vector
            applyMesh(rawMeshDataVec, uniqueVertices.size(), uniqueVertices.data(),
                      indices.size(), indices.data());
          }
        }

        //Copy indexed mesh data into a vector of RawMeshData
        void copyIndexedMeshes(const AmmoniteVertex** meshArray,
                               const unsigned int** indicesArray,
                               unsigned int meshCount,
                               const unsigned int* vertexCounts,
                               const unsigned int* indexCounts,
                               std::vector<RawMeshData>* rawMeshDataVec) {
          //Copy vertices and indices of already indexed mesh
          for (unsigned int meshIndex = 0; meshIndex < meshCount; meshIndex++) {
            const unsigned int vertexCount = vertexCounts[meshIndex];
            const unsigned int indexCount = indexCounts[meshIndex];
            const AmmoniteVertex* const mesh = meshArray[meshIndex];
            const unsigned int* const indices = indicesArray[meshIndex];

            //Copy the vertices and indices to the data vector
            applyMesh(rawMeshDataVec, vertexCount, mesh, indexCount, indices);
          }
        }

        /*
         - Load material components, returning a texture ID
         - Textures loads will be queued on the thread pool
         - Colour materials will be uploaded immediately
        */
        GLuint loadMaterialComponent(const AmmoniteMaterialComponent& component,
                                     bool isTexture) {
          if (isTexture) {
            return queueTextureLoad(*component.textureInfo.texturePath, false,
                                    component.textureInfo.isSrgbTexture);
          }

          return textures::internal::loadSolidTexture(component.colour);
        }


        //Load and apply each component of each material to its mesh
        void applyMaterials(ModelData* modelData, const AmmoniteMaterial* materials,
                            unsigned int meshCount) {
          for (unsigned int meshIndex = 0; meshIndex < meshCount; meshIndex++) {
            const AmmoniteMaterial& material = materials[meshIndex];
            modelData->textureIds.push_back({
              .diffuseId = loadMaterialComponent(material.diffuse,
                                                 material.diffuseIsTexture),
              .specularId = loadMaterialComponent(material.specular,
                                                  material.specularIsTexture)
            });
          }
        }
      }

      bool loadMemoryObject(ModelData* modelData,
                            std::vector<RawMeshData>* rawMeshDataVec,
                            const ModelLoadInfo& modelLoadInfo) {
        const ModelMemoryInfo& memoryInfo = modelLoadInfo.memoryInfo;

        //Load and apply the materials
        applyMaterials(modelData, memoryInfo.materials, memoryInfo.meshCount);

        //Upload mesh data, indexing if necessary
        if (modelLoadInfo.memoryInfo.indicesArray == nullptr) {
          indexMeshes(memoryInfo.meshArray, memoryInfo.meshCount,
                      memoryInfo.vertexCounts, rawMeshDataVec);
        } else {
          copyIndexedMeshes(memoryInfo.meshArray, memoryInfo.indicesArray,
                            memoryInfo.meshCount, memoryInfo.vertexCounts,
                            memoryInfo.indexCounts, rawMeshDataVec);
        }

        //Sync the queued texture loads
        uploadQueuedTextures();

        return true;
      }
    }
  }
}
