#include <cstdint>

#include "vectorTestTemplates.hpp"

namespace tests {
  bool testUInt32x3() {
    return testVector<uint32_t, 3>("uint32_t");
  }
}
