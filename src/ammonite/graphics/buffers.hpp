#ifndef INTERNALBUFFERS
#define INTERNALBUFFERS

#include <vector>

#include "../models/models.hpp"
#include "../visibility.hpp"

namespace AMMONITE_INTERNAL ammonite {
  namespace graphics {
    namespace internal {
      void createModelBuffers(models::internal::ModelData* modelData,
                              const std::vector<models::internal::RawMeshData>* rawMeshDataVec);
      void deleteModelBuffers(const models::internal::ModelData* modelData);

      void deleteLightBuffers();
      void uploadLightBuffers(const void* lightData, unsigned int lightDataSize,
                              const void* shadowData, unsigned int shadowDataSize);
    }
  }
}

#endif
