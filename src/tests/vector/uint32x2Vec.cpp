#include <cstdint>

#include "vectorTestTemplates.hpp"

namespace tests {
  bool testUInt32x2() {
    return testVector<uint32_t, 2>("uint32_t");
  }
}
