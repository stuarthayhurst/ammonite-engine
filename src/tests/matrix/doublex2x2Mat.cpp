#include "matrixTestTemplates.hpp"

namespace tests {
  bool testDoublex2x2() {
    return testMatrix<double, 2, 2>("double");
  }
}
