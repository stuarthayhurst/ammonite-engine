#include <cstdint>

#include "vectorTestTemplates.hpp"

namespace tests {
  bool testInt8x2() {
    return testVectors<int8_t, 2>("int8_t");
  }
}
