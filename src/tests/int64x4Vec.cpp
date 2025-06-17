#include <cstdint>

#include "vectorTestTemplates.hpp"

namespace tests {
  bool testInt64x4() {
    return testVectors<int64_t, 4>("int64_t");
  }
}
