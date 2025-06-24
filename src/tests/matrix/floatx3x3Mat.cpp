#include "matrixTestTemplates.hpp"

namespace tests {
  bool testFloatx3x3() {
    return testMatrix<float, 3, 3>("float");
  }
}
