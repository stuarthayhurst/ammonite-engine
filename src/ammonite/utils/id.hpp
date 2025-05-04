#ifndef INTERNALID
#define INTERNALID

#include <unordered_map>

#include "../internal.hpp"

//Include public interface
#include "../../include/ammonite/utils/id.hpp" // IWYU pragma: export

namespace ammonite {
  namespace utils {
    namespace AMMONITE_INTERNAL internal {
      /*
       - Find the next unused ID in tracker, starting with lastId
       - Write the result back to lastId, and return it
      */
      template <typename T>
      static AmmoniteId setNextId(AmmoniteId* lastId,
                                  const std::unordered_map<AmmoniteId, T>& tracker) {
        while (*lastId == 0 || tracker.contains(*lastId)) {
          (*lastId)++;
        }

        return (*lastId);
      }
    }
  }
}

#endif
