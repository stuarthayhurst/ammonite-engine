#include <cstdint>

#include "vectorTestTemplates.hpp"

namespace tests {
  bool testInt32x4() {
    return testVectors<int32_t, 4>("int32_t");
  }
}
