#include <cstdint>

#include "vectorTestTemplates.hpp"

namespace tests {
  bool testInt16x3() {
    return testVectors<int16_t, 3>("int16_t");
  }
}
