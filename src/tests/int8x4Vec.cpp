#include <cstdint>

#include "vectorTestTemplates.hpp"

namespace tests {
  bool testInt8x4() {
    return testVectors<int8_t, 4>("int8_t");
  }
}
