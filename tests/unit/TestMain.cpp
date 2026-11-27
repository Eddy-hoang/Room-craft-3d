#include <iostream>

// Khai báo các bộ unit test độc lập cho Milestone M1
int runTransformTests();
int runCameraTests();
int runMeshValidationTests();

int main()
{
    int failures = 0;

    std::cout << "================ RoomCraft 3D - M1 Unit Tests ================" << std::endl;
    failures += runTransformTests();
    failures += runCameraTests();
    failures += runMeshValidationTests();
    std::cout << "==============================================================" << std::endl;

    if (failures == 0) {
        std::cout << "[UNIT-TESTS] ALL PASSED (0 failures)." << std::endl;
        return 0;
    }

    std::cerr << "[UNIT-TESTS] FAILED with " << failures << " failure(s)." << std::endl;
    return 1;
}
