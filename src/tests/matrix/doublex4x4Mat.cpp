#include "matrixTestTemplates.hpp"

namespace tests {
  bool testDoublex4x4() {
    return testMatrix<double, 4, 4>("double");
  }
}
