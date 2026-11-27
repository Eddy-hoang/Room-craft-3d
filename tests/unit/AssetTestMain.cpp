#include <iostream>

int runModelLoaderTests();
int runAssetNormalizerTests();
int runAssetCatalogTests();

int main()
{
    int failures = 0;

    std::cout << "============== RoomCraft 3D - M3 Asset Unit Tests ==============" << std::endl;
    failures += runModelLoaderTests();
    failures += runAssetNormalizerTests();
    failures += runAssetCatalogTests();
    std::cout << "================================================================" << std::endl;

    if (failures == 0) {
        std::cout << "[M3-UNIT-TESTS] ALL PASSED (0 failures)." << std::endl;
        return 0;
    }

    std::cerr << "[M3-UNIT-TESTS] FAILED with " << failures << " failure(s)." << std::endl;
    return 1;
}
