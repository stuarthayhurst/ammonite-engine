#include "vectorTestTemplates.hpp"

namespace tests {
  bool testFloatx2() {
    return testVectors<float, 2>("float");
  }
}
