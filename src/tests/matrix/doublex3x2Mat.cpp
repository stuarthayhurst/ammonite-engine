#include "matrixTestTemplates.hpp"

namespace tests {
  bool testDoublex3x2() {
    return testMatrix<double, 3, 2>("double");
  }
}
