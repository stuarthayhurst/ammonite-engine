#include <cstdint>

#include "vectorTestTemplates.hpp"

namespace tests {
  bool testInt32x2() {
    return testVector<int32_t, 2>("int32_t");
  }
}
