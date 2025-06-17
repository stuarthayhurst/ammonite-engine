#include <cstdint>

#include "vectorTestTemplates.hpp"

namespace tests {
  bool testUInt16x2() {
    return testVectors<uint16_t, 2>("uint16_t");
  }
}
