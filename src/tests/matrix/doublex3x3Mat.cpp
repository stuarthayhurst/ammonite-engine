#include "matrixTestTemplates.hpp"

namespace tests {
  bool testDoublex3x3() {
    return testMatrix<double, 3, 3>("double");
  }
}
