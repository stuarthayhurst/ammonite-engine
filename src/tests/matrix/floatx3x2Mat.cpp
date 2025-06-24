#include "matrixTestTemplates.hpp"

namespace tests {
  bool testFloatx3x2() {
    return testMatrix<float, 3, 2>("float");
  }
}
