#include "matrixTestTemplates.hpp"

namespace tests {
  bool testFloatx4x3() {
    return testMatrix<float, 4, 3>("float");
  }
}
