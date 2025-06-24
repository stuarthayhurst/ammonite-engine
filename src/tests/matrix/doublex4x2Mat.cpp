#include "matrixTestTemplates.hpp"

namespace tests {
  bool testDoublex4x2() {
    return testMatrix<double, 4, 2>("double");
  }
}
