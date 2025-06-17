#include <cstdint>

#include "vectorTestTemplates.hpp"

namespace tests {
  bool testInt16x4() {
    return testVectors<int16_t, 4>("int16_t");
  }
}
