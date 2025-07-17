#include "quaternionTestTemplates.hpp"

namespace tests {
  bool testFloatQuat() {
    return testQuaternion<float>("float");
  }
}
