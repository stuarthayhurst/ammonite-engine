#include <cstdint>

#include "vectorTestTemplates.hpp"

namespace tests {
  bool testUInt16x3() {
    return testVectors<uint16_t, 3>("uint16_t");
  }
}
