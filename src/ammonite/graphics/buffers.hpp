#ifndef INTERNALBUFFERS
#define INTERNALBUFFERS

#include <vector>

#include "../models/models.hpp"
#include "../visibility.hpp"

namespace AMMONITE_INTERNAL ammonite {
  namespace graphics {
    namespace internal {
      void createModelBuffers(models::internal::ModelData* modelData,
                              std::vector<models::internal::RawMeshData>* rawMeshDataVec);
      void deleteModelBuffers(models::internal::ModelData* modelData);

      void deleteLightBuffers();
      void uploadLightBuffers(void* lightData, unsigned int lightDataSize,
                              void* shadowData, unsigned int shadowDataSize);
    }
  }
}

#endif
