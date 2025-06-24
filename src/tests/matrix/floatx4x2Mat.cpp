#include "matrixTestTemplates.hpp"

namespace tests {
  bool testFloatx4x2() {
    return testMatrix<float, 4, 2>("float");
  }
}
