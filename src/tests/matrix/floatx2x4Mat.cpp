#include "matrixTestTemplates.hpp"

namespace tests {
  bool testFloatx2x4() {
    return testMatrix<float, 2, 4>("float");
  }
}
