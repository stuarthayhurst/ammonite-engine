#include <cstdint>

#include "vectorTestTemplates.hpp"

namespace tests {
  bool testUInt64x4() {
    return testVectors<uint64_t, 4>("int64_t");
  }
}
