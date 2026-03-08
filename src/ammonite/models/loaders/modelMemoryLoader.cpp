#include <cstring>
#include <iostream>
#include <map>
#include <vector>

#include "../models.hpp"

#include "../../graphics/textures.hpp"
#include "../../utils/debug.hpp"
#include "../../utils/logging.hpp"
#include "../../utils/thread.hpp"


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
        //NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
        struct IndexThreadData {
          const unsigned int vertexCount;
          const AmmoniteVertex* const mesh;
          RawMeshData* rawMeshDataPtr;
        };
        //NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)

        //Vector to store thread data between calls
        std::vector<IndexThreadData> indexThreadData;
      }

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

        /*
         - Copy the mesh data into the raw mesh data vector
         - This must be thread-safe
        */
        void applyMesh(RawMeshData* rawMeshDataPtr, unsigned int vertexCount, const AmmoniteVertex* vertexData, unsigned int indexCount, const unsigned int* indices) {
          //Copy vertices
          rawMeshDataPtr->vertexCount = vertexCount;
          rawMeshDataPtr->vertexData = new AmmoniteVertex[vertexCount];
          std::memcpy(rawMeshDataPtr->vertexData, vertexData, sizeof(AmmoniteVertex) * vertexCount);

          //Copy indices
          rawMeshDataPtr->indexCount = indexCount;
          rawMeshDataPtr->indices = new unsigned int[indexCount];
          std::memcpy(rawMeshDataPtr->indices, indices, sizeof(unsigned int) * indexCount);
        }

        void modelIndexJob(void* userPtr) {
          const IndexThreadData* const dataPtr = (IndexThreadData*)userPtr;

          //Store the generated indices and unique vertices
          std::vector<AmmoniteVertex> uniqueVertices;
          std::vector<unsigned int> indices;
          indices.reserve(dataPtr->vertexCount);

          //Generate indices and unique vertices for the mesh
          std::map<AmmoniteVertex, unsigned int, VertexCompare> vertexIndexMap;
          unsigned int nextIndex = 0;
          for (unsigned int i = 0; i < dataPtr->vertexCount; i++) {
            const AmmoniteVertex& vertexData = dataPtr->mesh[i];

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

          //Copy the vertices and indices to the data vector entry
          applyMesh(dataPtr->rawMeshDataPtr, uniqueVertices.size(),
                    uniqueVertices.data(), indices.size(), indices.data());
        }

        void syncMeshIndexing(AmmoniteGroup* indexGroup, unsigned int meshCount) {
          ammonite::utils::thread::waitGroupComplete(indexGroup, meshCount);
          indexThreadData.clear();
        }

        /*
         - Index then copy mesh data into a vector of RawMeshData
           - Use the thread pool
         - Call syncMeshIndexing() to wait for this to complete and clean up
         - Return the number of jobs to wait for
         - This should only be called once per mesh data vector
        */
        unsigned int indexMeshes(const AmmoniteVertex** meshArray,
                                 unsigned int meshCount,
                                 const unsigned int* vertexCounts,
                                 std::vector<RawMeshData>* rawMeshDataVec,
                                 AmmoniteGroup* indexGroupPtr) {
          //Reserve space in the vector of thread data
          indexThreadData.reserve(meshCount);

          //Reserve space for the mesh data
          rawMeshDataVec->resize(meshCount);

          //Prepare the data for indexing
          for (unsigned int meshIndex = 0; meshIndex < meshCount; meshIndex++) {
            indexThreadData.push_back({
              .vertexCount = vertexCounts[meshIndex],
              .mesh = meshArray[meshIndex],
              .rawMeshDataPtr = &rawMeshDataVec->at(meshIndex)
            });
          }

          //Index and upload each mesh, using the thread pool
          ammonite::utils::thread::submitMultiple(modelIndexJob, indexThreadData.data(),
                                                  sizeof(IndexThreadData), indexGroupPtr,
                                                  meshCount, nullptr);
          return meshCount;
        }

        //Copy indexed mesh data into a vector of RawMeshData
        unsigned int copyIndexedMeshes(const AmmoniteVertex** indexedMeshArray,
                                       const unsigned int** indicesArray,
                                       unsigned int meshCount,
                                       const unsigned int* vertexCounts,
                                       const unsigned int* indexCounts,
                                       std::vector<RawMeshData>* rawMeshDataVec) {
          //Copy vertices and indices of already indexed mesh
          for (unsigned int meshIndex = 0; meshIndex < meshCount; meshIndex++) {
            const unsigned int vertexCount = vertexCounts[meshIndex];
            const unsigned int indexCount = indexCounts[meshIndex];
            const AmmoniteVertex* const indexedMesh = indexedMeshArray[meshIndex];
            const unsigned int* const indices = indicesArray[meshIndex];

            //Copy the vertices and indices to the data vector
            applyMesh(&rawMeshDataVec->emplace_back(), vertexCount, indexedMesh,
                      indexCount, indices);
          }

          return 0;
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

      //Mesh reindexing output helpers
      namespace {
        /*
         - Compare the original vertex count against the deindexed count
         - This should never shrink, but may stay the same
        */
        void debugPrintMeshSizes(const ModelMemoryInfo& indexedInfo,
                                 const ModelMemoryInfo& deindexedInfo) {
          //Print the original vertex counts
          ammoniteInternalDebug << "Indexed vertex counts:" << std::endl;
          for (unsigned int i = 0; i < indexedInfo.meshCount; i++) {
            ammoniteInternalDebug << " - " << indexedInfo.vertexCounts[i] << std::endl;
          }

          //Print the expanded vertex counts
          ammoniteInternalDebug << "Deindexed vertex counts:" << std::endl;
          for (unsigned int i = 0; i < deindexedInfo.meshCount; i++) {
            ammoniteInternalDebug << " - " << deindexedInfo.vertexCounts[i] << std::endl;
          }

          //Check that no meshes were dropped
          if (indexedInfo.meshCount != deindexedInfo.meshCount) {
            ammonite::utils::warning << "Mesh count changed from " \
                                     << indexedInfo.meshCount << " to " \
                                     << deindexedInfo.meshCount << std::endl;
          }
        }
      }

      //Helpers to deindex an indexed mesh array
      namespace {
        //Allocate space for a deindexed mesh, using an indexed mesh
        AmmoniteVertex** allocateDeindexedMesh(unsigned int meshCount,
                                               const unsigned int* indexCounts) {
          //Allocate an array for the meshes
          AmmoniteVertex** const meshArray = new AmmoniteVertex*[meshCount];

          //Allocate each mesh
          for (unsigned int meshIndex = 0; meshIndex < meshCount; meshIndex++) {
            meshArray[meshIndex] = new AmmoniteVertex[indexCounts[meshIndex]];
          }

          return meshArray;
        }

        //Free a deindexed mesh
        void freeDeindexedMesh(AmmoniteVertex** meshArray, unsigned int meshCount) {
          for (unsigned int meshIndex = 0; meshIndex < meshCount; meshIndex++) {
            delete [] meshArray[meshIndex];
          }

          delete [] meshArray;
        }

        /*
         - Deindex an indexed mesh array
         - Use allocateDeindexedMesh() to allocate space for the output
        */
        void deindexMesh(const AmmoniteVertex** indexedMeshArray,
                         const unsigned int** indicesArray,
                         unsigned int meshCount,
                         const unsigned int* indexCounts,
                         AmmoniteVertex** meshArray) {
          for (unsigned int meshIndex = 0; meshIndex < meshCount; meshIndex++) {
            const unsigned int indexCount = indexCounts[meshIndex];
            const AmmoniteVertex* const indexedMesh = indexedMeshArray[meshIndex];
            const unsigned int* const meshIndices = indicesArray[meshIndex];

            //Use the indices to find the correct vertex
            AmmoniteVertex* const mesh = meshArray[meshIndex];
            for (unsigned int i = 0; i < indexCount; i++) {
              mesh[i] = indexedMesh[meshIndices[i]];
            }
          }
        }
      }

      /*
       - Same as loadMemoryObject, but reindex indexed meshes
       - Use this for debugging the mesh indexing, or checking the quality of
         indexed meshes in memory
      */
      bool reindexLoadMemoryObject(ModelData* modelData,
                                  std::vector<RawMeshData>* rawMeshDataVec,
                                  const ModelLoadInfo& modelLoadInfo) {
        const ModelMemoryInfo& memoryInfo = modelLoadInfo.memoryInfo;

        //Pass data through if it isn't indexed
        if (modelLoadInfo.memoryInfo.indicesArray == nullptr) {
          return loadMemoryObject(modelData, rawMeshDataVec, modelLoadInfo);
        }

        //Deindex and hand off to loadMemoryObject() to be reindexed
        AmmoniteVertex** const deindexedMeshArray = allocateDeindexedMesh(
          memoryInfo.meshCount, memoryInfo.indexCounts);
        deindexMesh(memoryInfo.meshArray, memoryInfo.indicesArray,
                    memoryInfo.meshCount, memoryInfo.indexCounts, deindexedMeshArray);

        //Generate new load info for deindexed data
        const ModelLoadInfo deindexedModelLoadInfo = {
          .memoryInfo = {
            .meshArray = (const AmmoniteVertex**)deindexedMeshArray,
            .indicesArray = nullptr,
            .materials = memoryInfo.materials,
            .meshCount = memoryInfo.meshCount,
            .vertexCounts = memoryInfo.indexCounts,
            .indexCounts = nullptr
          },
          .isFileBased = false
        };

        //Check reindexed mesh sizes
        debugPrintMeshSizes(memoryInfo, deindexedModelLoadInfo.memoryInfo);

        //Hand off to regular memory model loading
        const bool success = loadMemoryObject(modelData, rawMeshDataVec,
                                              deindexedModelLoadInfo);
        freeDeindexedMesh(deindexedMeshArray, memoryInfo.meshCount);

        return success;
      }

      /*
       - Process modelLoadInfo into modelData and rawMeshDataVec
       - Meshes will be indexed if not supplied as indexed meshes
       - Materials will be loaded
      */
      bool loadMemoryObject(ModelData* modelData,
                            std::vector<RawMeshData>* rawMeshDataVec,
                            const ModelLoadInfo& modelLoadInfo) {
        const ModelMemoryInfo& memoryInfo = modelLoadInfo.memoryInfo;

        //Load and apply the materials
        applyMaterials(modelData, memoryInfo.materials, memoryInfo.meshCount);

        //Upload mesh data, indexing if necessary
        AmmoniteGroup indexGroup{0};
        unsigned int jobSyncCount = 0;
        if (modelLoadInfo.memoryInfo.indicesArray == nullptr) {
          jobSyncCount = indexMeshes(memoryInfo.meshArray,
            memoryInfo.meshCount, memoryInfo.vertexCounts, rawMeshDataVec,
            &indexGroup);
        } else {
          jobSyncCount = copyIndexedMeshes(memoryInfo.meshArray,
            memoryInfo.indicesArray, memoryInfo.meshCount,
            memoryInfo.vertexCounts, memoryInfo.indexCounts, rawMeshDataVec);
        }

        //Sync the queued texture loads
        uploadQueuedTextures();

        //Sync the mesh indexing
        syncMeshIndexing(&indexGroup, jobSyncCount);

        return true;
      }
    }
  }
}
