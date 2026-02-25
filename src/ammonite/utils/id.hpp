#ifndef INTERNALID
#define INTERNALID

#include <unordered_map>

#include "../visibility.hpp"

//Include public interface
#include "../../include/ammonite/utils/id.hpp" // IWYU pragma: export

namespace AMMONITE_INTERNAL ammonite {
  namespace utils {
    namespace internal {
      /*
       - Find the next unused ID in tracker, starting with lastId + 1
       - Write the result back to lastId, and return it
      */
      template <typename T>
      static AmmoniteId setNextId(AmmoniteId* lastId,
                                  const std::unordered_map<AmmoniteId, T>& tracker) {
        (*lastId)++;
        while (tracker.contains(*lastId) || *lastId == 0) {
          (*lastId)++;
        }

        return (*lastId);
      }
    }
  }
}

#endif
