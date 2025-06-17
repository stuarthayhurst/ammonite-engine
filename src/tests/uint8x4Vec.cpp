#include <cstdint>

#include "vectorTestTemplates.hpp"

namespace tests {
  bool testUInt8x4() {
    return testVectors<uint8_t, 4>("uint8_t");
  }
}
