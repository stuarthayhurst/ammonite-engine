#include "vectorTestTemplates.hpp"

namespace tests {
  bool testFloatx4() {
    return testVectors<float, 4>("float");
  }
}
