#include <cstdint>

#include "vectorTestTemplates.hpp"

namespace tests {
  bool testInt64x2() {
    return testVector<int64_t, 2>("int64_t");
  }
}
