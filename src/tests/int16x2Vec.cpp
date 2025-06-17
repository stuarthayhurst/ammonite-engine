#include <cstdint>

#include "vectorTestTemplates.hpp"

namespace tests {
  bool testInt16x2() {
    return testVectors<int16_t, 2>("int16_t");
  }
}
