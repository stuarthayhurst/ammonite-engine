#include "vectorTestTemplates.hpp"

namespace tests {
  bool testFloatx3() {
    return testVector<float, 3>("float");
  }
}
