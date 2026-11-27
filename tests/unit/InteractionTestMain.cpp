#include <iostream>

int runRayTests();
int runIntersectionTests();
int runCameraControllerTests();
int runPickingTests();
int runInteractionTests();

int main()
{
    std::cout << "================ RoomCraft 3D - M5 Interaction Unit Tests (CPU) ================" << std::endl;

    int failures = 0;
    failures += runRayTests();
    failures += runIntersectionTests();
    failures += runCameraControllerTests();
    failures += runPickingTests();
    failures += runInteractionTests();

    std::cout << "================================================================================" << std::endl;
    if (failures == 0) {
        std::cout << "[M5-UNIT-TESTS] ALL PASSED (0 failures)." << std::endl;
        return 0;
    }

    std::cerr << "[M5-UNIT-TESTS] FAILED with " << failures << " failure(s)." << std::endl;
    return 1;
}
