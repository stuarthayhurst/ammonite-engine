#include <cstdint>

#include "vectorTestTemplates.hpp"

namespace tests {
  bool testInt8x3() {
    return testVectors<int8_t, 3>("int8_t");
  }
}
