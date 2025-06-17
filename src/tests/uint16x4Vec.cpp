#include <cstdint>

#include "vectorTestTemplates.hpp"

namespace tests {
  bool testUInt16x4() {
    return testVectors<uint16_t, 4>("uint16_t");
  }
}
