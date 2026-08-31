#ifndef INTERNALENGINE
#define INTERNALENGINE

#include "visibility.hpp"

//Include public interface
#include "../include/ammonite/engine.hpp" // IWYU pragma: export

namespace AMMONITE_INTERNAL ammonite {
  namespace internal {
    void* getThreadPoolInstance();
  }
}

#endif
