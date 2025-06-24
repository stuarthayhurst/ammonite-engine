#include "matrixTestTemplates.hpp"

namespace tests {
  bool testDoublex2x3() {
    return testMatrix<double, 2, 3>("double");
  }
}
