#ifndef INTERNALBUFFERS
#define INTERNALBUFFERS

#include <vector>

#include "../models/models.hpp"
#include "../visibility.hpp"

namespace ammonite {
  namespace graphics {
    namespace AMMONITE_INTERNAL internal {
      void createModelBuffers(models::internal::ModelData* modelData,
                              std::vector<models::internal::RawMeshData>* rawMeshDataVec);
      void deleteModelBuffers(models::internal::ModelData* modelData);
    }
  }
}

#endif
