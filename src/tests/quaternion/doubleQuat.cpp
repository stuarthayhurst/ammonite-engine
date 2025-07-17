#include "quaternionTestTemplates.hpp"

namespace tests {
  bool testDoubleQuat() {
    return testQuaternion<double>("double");
  }
}
