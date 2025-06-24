#include <cstdint>

#include "vectorTestTemplates.hpp"

namespace tests {
  bool testUInt64x2() {
    return testVector<uint64_t, 2>("int64_t");
  }
}
