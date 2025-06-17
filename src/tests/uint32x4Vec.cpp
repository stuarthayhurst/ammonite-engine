#include <cstdint>

#include "vectorTestTemplates.hpp"

namespace tests {
  bool testUInt32x4() {
    return testVectors<uint32_t, 4>("uint32_t");
  }
}
