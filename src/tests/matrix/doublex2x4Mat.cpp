#include "matrixTestTemplates.hpp"

namespace tests {
  bool testDoublex2x4() {
    return testMatrix<double, 2, 4>("double");
  }
}
