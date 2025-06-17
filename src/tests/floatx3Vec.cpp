#include "vectorTestTemplates.hpp"

namespace tests {
  bool testFloatx3() {
    return testVectors<float, 3>("float");
  }
}
