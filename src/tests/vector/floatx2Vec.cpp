#include "vectorTestTemplates.hpp"

namespace tests {
  bool testFloatx2() {
    return testVector<float, 2>("float");
  }
}
