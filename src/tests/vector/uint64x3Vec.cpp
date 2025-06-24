#include <cstdint>

#include "vectorTestTemplates.hpp"

namespace tests {
  bool testUInt64x3() {
    return testVector<uint64_t, 3>("int64_t");
  }
}
