#include "matrixTestTemplates.hpp"

namespace tests {
  bool testFloatx3x4() {
    return testMatrix<float, 3, 4>("float");
  }
}
