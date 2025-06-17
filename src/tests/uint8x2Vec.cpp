#include <cstdint>

#include "vectorTestTemplates.hpp"

namespace tests {
  bool testUInt8x2() {
    return testVectors<uint8_t, 2>("uint8_t");
  }
}
