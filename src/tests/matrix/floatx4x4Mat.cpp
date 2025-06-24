#include "matrixTestTemplates.hpp"

namespace tests {
  bool testFloatx4x4() {
    return testMatrix<float, 4, 4>("float");
  }
}
