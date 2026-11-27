#include <iostream>

int runPrimitiveTests();
int runBezierCurveTests();
int runBezierSurfaceTests();
int runRevolutionSurfaceTests();
int runBoundingBoxTests();
int runFurnitureTests();

int main()
{
    int failures = 0;

    std::cout << "============= RoomCraft 3D - M2 Geometry Unit Tests =============" << std::endl;
    failures += runPrimitiveTests();
    failures += runBezierCurveTests();
    failures += runBezierSurfaceTests();
    failures += runRevolutionSurfaceTests();
    failures += runBoundingBoxTests();
    failures += runFurnitureTests();
    std::cout << "=================================================================" << std::endl;

    if (failures == 0) {
        std::cout << "[M2-UNIT-TESTS] ALL PASSED (0 failures)." << std::endl;
        return 0;
    }

    std::cerr << "[M2-UNIT-TESTS] FAILED with " << failures << " failure(s)." << std::endl;
    return 1;
}
