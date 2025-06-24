#include "matrixTestTemplates.hpp"

namespace tests {
  bool testFloatx2x3() {
    return testMatrix<float, 2, 3>("float");
  }
}
