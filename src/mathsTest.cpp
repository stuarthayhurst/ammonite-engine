#include <cstdlib>
#include <iostream>

#include <ammonite/ammonite.hpp>

#include "tests/matrix/matrixTestTemplates.hpp"
#include "tests/quaternion/quaternionTestTemplates.hpp"
#include "tests/vector/vectorTestTemplates.hpp"

using TestFunction = bool (*)();

//Helpers
namespace {
  bool runTest(TestFunction testFunction, double& typeTime, double& structTime) {
    //Run the test and time it
    const ammonite::utils::Timer testTimer;
    const bool passed = testFunction();
    const double testTime = testTimer.getTime();

    ammonite::utils::normal << "  Completed in " << testTime << "s" << std::endl;
    typeTime += testTime;
    structTime += testTime;

    return passed;
  }
}

//Test data and accumulators
namespace {
  double int32Time = 0.0;
  double int64Time = 0.0;
  double uint32Time = 0.0;
  double uint64Time = 0.0;
  double floatTime = 0.0;
  double doubleTime = 0.0;

  double vectorTime = 0.0;
  double matrixTime = 0.0;
  double quatTime = 0.0;

  //NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
  struct TestGroup {
    TestFunction function;
    double& typeTime;
    double& structTime;
  } testGroups[] = {
    //Vectors
    {tests::testInt32x2, int32Time, vectorTime},
    {tests::testInt64x2, int64Time, vectorTime},
    {tests::testUInt32x2, uint32Time, vectorTime},
    {tests::testUInt64x2, uint64Time, vectorTime},
    {tests::testFloatx2, floatTime, vectorTime},
    {tests::testDoublex2, doubleTime, vectorTime},

    {tests::testInt32x3, int32Time, vectorTime},
    {tests::testInt64x3, int64Time, vectorTime},
    {tests::testUInt32x3, uint32Time, vectorTime},
    {tests::testUInt64x3, uint64Time, vectorTime},
    {tests::testFloatx3, floatTime, vectorTime},
    {tests::testDoublex3, doubleTime, vectorTime},

    {tests::testInt32x4, int32Time, vectorTime},
    {tests::testInt64x4, int64Time, vectorTime},
    {tests::testUInt32x4, uint32Time, vectorTime},
    {tests::testUInt64x4, uint64Time, vectorTime},
    {tests::testFloatx4, floatTime, vectorTime},
    {tests::testDoublex4, doubleTime, vectorTime},

    //Matrices
    {tests::testFloatx2x2, floatTime, matrixTime},
    {tests::testFloatx2x3, floatTime, matrixTime},
    {tests::testFloatx2x4, floatTime, matrixTime},
    {tests::testDoublex2x2, doubleTime, matrixTime},
    {tests::testDoublex2x3, doubleTime, matrixTime},
    {tests::testDoublex2x4, doubleTime, matrixTime},

    {tests::testFloatx3x2, floatTime, matrixTime},
    {tests::testFloatx3x3, floatTime, matrixTime},
    {tests::testFloatx3x4, floatTime, matrixTime},
    {tests::testDoublex3x2, doubleTime, matrixTime},
    {tests::testDoublex3x3, doubleTime, matrixTime},
    {tests::testDoublex3x4, doubleTime, matrixTime},

    {tests::testFloatx4x2, floatTime, matrixTime},
    {tests::testFloatx4x3, floatTime, matrixTime},
    {tests::testFloatx4x4, floatTime, matrixTime},
    {tests::testDoublex4x2, doubleTime, matrixTime},
    {tests::testDoublex4x3, doubleTime, matrixTime},
    {tests::testDoublex4x4, doubleTime, matrixTime},

    //Quaternions
    {tests::testFloatQuat, floatTime, quatTime},
    {tests::testDoubleQuat, doubleTime, quatTime}
  };
  //NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
}

int main() {
  //Run the tests and accumulate times
  bool passed = true;
  for (TestGroup& testGroup : testGroups) {
    passed &= runTest(testGroup.function, testGroup.typeTime, testGroup.structTime);
  }

  if (!passed) {
    ammonite::utils::normal << std::endl;
    ammonite::utils::error << "Vector / matrix / quaternion tests failed" << std::endl;
    return EXIT_FAILURE;
  }

  //Print data type accumulated times
  ammonite::utils::normal << std::endl;
  ammonite::utils::normal << "int32_t total time: " << int32Time << "s" << std::endl;
  ammonite::utils::normal << "int64_t total time: " << int64Time << "s" << std::endl;
  ammonite::utils::normal << "uint32_t total time: " << uint32Time << "s" << std::endl;
  ammonite::utils::normal << "uint64_t total time: " << uint64Time << "s" << std::endl;
  ammonite::utils::normal << "float total time: " << floatTime << "s" << std::endl;
  ammonite::utils::normal << "double total time: " << doubleTime << "s" << std::endl;

  //Print structure type accumulated times
  ammonite::utils::normal << std::endl;
  ammonite::utils::normal << "Vector total time: " << vectorTime << "s" << std::endl;
  ammonite::utils::normal << "Matrix total time: " << matrixTime << "s" << std::endl;
  ammonite::utils::normal << "Quaternion total time: " << quatTime << "s" << std::endl;

  //Print final totals
  ammonite::utils::normal << std::endl;
  ammonite::utils::normal << "Total time: " << vectorTime + matrixTime + quatTime \
                          << "s" << std::endl;

  return EXIT_SUCCESS;
}
