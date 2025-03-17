#ifndef INTERNALID
#define INTERNALID

#include <map>

#include "../internal.hpp"
#include "../types.hpp"

namespace ammonite {
  namespace utils {
    namespace AMMONITE_INTERNAL internal {
      /*
       - Find the next unused ID in tracker, starting with lastId
       - Write the result back to lastId, and return it
      */
      template <typename T>
      AmmoniteId setNextId(AmmoniteId* lastId, const std::map<AmmoniteId, T>& tracker) {
        while (*lastId == 0 || tracker.contains(*lastId)) {
          (*lastId)++;
        }

        return (*lastId);
      }
    }
  }
}

#endif
