#include "vectorTestTemplates.hpp"

namespace tests {
  bool testFloatx4() {
    return testVector<float, 4>("float");
  }
}
