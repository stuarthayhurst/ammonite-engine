#include <cstdlib>
#include <iostream>

#include <ammonite/ammonite.hpp>

#include "tests/matrix/matrixTestTemplates.hpp"
#include "tests/vector/vectorTestTemplates.hpp"

int main() {
  bool passed = true;

  passed &= tests::testInt32x2();
  passed &= tests::testInt64x2();
  passed &= tests::testUInt32x2();
  passed &= tests::testUInt64x2();
  passed &= tests::testFloatx2();
  passed &= tests::testDoublex2();

  passed &= tests::testInt32x3();
  passed &= tests::testInt64x3();
  passed &= tests::testUInt32x3();
  passed &= tests::testUInt64x3();
  passed &= tests::testFloatx3();
  passed &= tests::testDoublex3();

  passed &= tests::testInt32x4();
  passed &= tests::testInt64x4();
  passed &= tests::testUInt32x4();
  passed &= tests::testUInt64x4();
  passed &= tests::testFloatx4();
  passed &= tests::testDoublex4();

  passed &= tests::testFloatx2x2();
  passed &= tests::testFloatx2x3();
  passed &= tests::testFloatx2x4();
  passed &= tests::testDoublex2x2();
  passed &= tests::testDoublex2x3();
  passed &= tests::testDoublex2x4();

  passed &= tests::testFloatx3x2();
  passed &= tests::testFloatx3x3();
  passed &= tests::testFloatx3x4();
  passed &= tests::testDoublex3x2();
  passed &= tests::testDoublex3x3();
  passed &= tests::testDoublex3x4();

  passed &= tests::testFloatx4x2();
  passed &= tests::testFloatx4x3();
  passed &= tests::testFloatx4x4();
  passed &= tests::testDoublex4x2();
  passed &= tests::testDoublex4x3();
  passed &= tests::testDoublex4x4();

  if (!passed) {
    ammonite::utils::normal << std::endl;
    ammonite::utils::error << "Vector / matrix tests failed" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
