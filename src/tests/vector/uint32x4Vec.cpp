#include <cstdint>

#include "vectorTestTemplates.hpp"

namespace tests {
  bool testUInt32x4() {
    return testVector<uint32_t, 4>("uint32_t");
  }
}
