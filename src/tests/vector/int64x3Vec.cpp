#include <cstdint>

#include "vectorTestTemplates.hpp"

namespace tests {
  bool testInt64x3() {
    return testVector<int64_t, 3>("int64_t");
  }
}
