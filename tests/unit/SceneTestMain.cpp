#include <iostream>

int runRoomTests();
int runSceneTests();
int runPlacementTests();
int runScenePresetTests();

int main()
{
    std::cout << "================ RoomCraft 3D - M4 Scene Unit Tests (CPU) ================" << std::endl;

    int failures = 0;
    failures += runRoomTests();
    failures += runSceneTests();
    failures += runPlacementTests();
    failures += runScenePresetTests();

    std::cout << "==========================================================================" << std::endl;
    if (failures == 0) {
        std::cout << "[M4-UNIT-TESTS] ALL PASSED (0 failures)." << std::endl;
        return 0;
    }

    std::cerr << "[M4-UNIT-TESTS] FAILED with " << failures << " failure(s)." << std::endl;
    return 1;
}
