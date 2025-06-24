#include "matrixTestTemplates.hpp"

namespace tests {
  bool testFloatx2x2() {
    return testMatrix<float, 2, 2>("float");
  }
}
