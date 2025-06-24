#include "matrixTestTemplates.hpp"

namespace tests {
  bool testDoublex4x3() {
    return testMatrix<double, 4, 3>("double");
  }
}
