#include <iostream>

#include <ammonite/ammonite.hpp>

#include "tests/vectorTestTemplates.hpp"

int main() {
  bool passed = true;

  passed &= tests::testInt8x2();
  passed &= tests::testInt16x2();
  passed &= tests::testInt32x2();
  passed &= tests::testInt64x2();
  passed &= tests::testUInt8x2();
  passed &= tests::testUInt16x2();
  passed &= tests::testUInt32x2();
  passed &= tests::testUInt64x2();
  passed &= tests::testFloatx2();
  passed &= tests::testDoublex2();

  passed &= tests::testInt8x3();
  passed &= tests::testInt16x3();
  passed &= tests::testInt32x3();
  passed &= tests::testInt64x3();
  passed &= tests::testUInt8x3();
  passed &= tests::testUInt16x3();
  passed &= tests::testUInt32x3();
  passed &= tests::testUInt64x3();
  passed &= tests::testFloatx3();
  passed &= tests::testDoublex3();

  passed &= tests::testInt8x4();
  passed &= tests::testInt16x4();
  passed &= tests::testInt32x4();
  passed &= tests::testInt64x4();
  passed &= tests::testUInt8x4();
  passed &= tests::testUInt16x4();
  passed &= tests::testUInt32x4();
  passed &= tests::testUInt64x4();
  passed &= tests::testFloatx4();
  passed &= tests::testDoublex4();

  if (!passed) {
    ammonite::utils::normal << std::endl;
    ammonite::utils::error << "Vector tests failed" << std::endl;
    return 1;
  }

  return 0;
}
