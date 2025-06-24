#include <cstdint>

#include "vectorTestTemplates.hpp"

namespace tests {
  bool testInt32x3() {
    return testVector<int32_t, 3>("int32_t");
  }
}
