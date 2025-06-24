#include "matrixTestTemplates.hpp"

namespace tests {
  bool testDoublex3x4() {
    return testMatrix<double, 3, 4>("double");
  }
}
