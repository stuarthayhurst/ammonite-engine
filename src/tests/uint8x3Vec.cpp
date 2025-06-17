#include <cstdint>

#include "vectorTestTemplates.hpp"

namespace tests {
  bool testUInt8x3() {
    return testVectors<uint8_t, 3>("uint8_t");
  }
}
