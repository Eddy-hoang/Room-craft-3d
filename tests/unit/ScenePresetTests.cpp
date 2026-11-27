#include "roomcraft/assets/FurnitureCatalog.hpp"
#include "roomcraft/scene/Scene.hpp"
#include "roomcraft/scene/ScenePresets.hpp"

#include <cmath>
#include <iostream>
#include <string>

namespace {

bool approx(float a, float b, float eps = 1e-3f)
{
    return std::abs(a - b) <= eps;
}

bool check(bool cond, const char* label, int& failures)
{
    std::cout << "  [Presets] " << label << ": " << (cond ? "PASS" : "FAIL") << std::endl;
    if (!cond) ++failures;
    return cond;
}

} // namespace

int runScenePresetTests()
{
    int failures = 0;
    roomcraft::assets::FurnitureCatalog catalog;
    roomcraft::scene::Scene scene;
    std::string err;

    // 1. Modern Bedroom (5 x 6 x 3 m, >= 7 objects, Desk Lamp at y = 0.75, Carpet at y = 0.003)
    const bool bedroomOk = roomcraft::scene::ScenePresets::applyPreset(
        roomcraft::scene::ScenePresetType::ModernBedroom, scene, catalog, &err);
    bool bedroomLampOnDesk = false;
    bool bedroomCarpetOffset = false;
    for (const auto& obj : scene.getObjects()) {
        if (obj.assetId == "procedural-desk-lamp" && approx(obj.transform.position.y, 0.75f)) {
            bedroomLampOnDesk = true;
        }
        if (obj.assetId == "blender-carpet" && obj.transform.position.y > 0.001f) {
            bedroomCarpetOffset = true;
        }
    }
    check(bedroomOk && approx(scene.getRoom().getSettings().width, 5.0f) &&
          approx(scene.getRoom().getSettings().length, 6.0f) &&
          scene.getObjectCount() == 7 && bedroomLampOnDesk && bedroomCarpetOffset,
          "TC-M4-24 Modern Bedroom preset valid (5x6x3m, 7 objects, desk lamp on desk, carpet offset)", failures);

    // 2. Gaming Workspace (4 x 5 x 3 m, Computer Setup & Desk Lamp at y = 0.75)
    const bool gamingOk = roomcraft::scene::ScenePresets::applyPreset(
        roomcraft::scene::ScenePresetType::GamingWorkspace, scene, catalog, &err);
    bool pcOnDesk = false;
    for (const auto& obj : scene.getObjects()) {
        if (obj.assetId == "blender-computer-setup" && approx(obj.transform.position.y, 0.75f)) {
            pcOnDesk = true;
        }
    }
    check(gamingOk && approx(scene.getRoom().getSettings().width, 4.0f) &&
          approx(scene.getRoom().getSettings().length, 5.0f) &&
          scene.getObjectCount() == 7 && pcOnDesk,
          "TC-M4-24 Gaming Workspace preset valid (4x5x3m, 7 objects, computer on desk y=0.75)", failures);

    // 3. Minimalist Living Room (6 x 7 x 3 m, Vase on coffee table y = 0.45)
    const bool livingOk = roomcraft::scene::ScenePresets::applyPreset(
        roomcraft::scene::ScenePresetType::MinimalistLivingRoom, scene, catalog, &err);
    bool vaseOnTable = false;
    for (const auto& obj : scene.getObjects()) {
        if (obj.assetId == "procedural-vase" && approx(obj.transform.position.y, 0.45f)) {
            vaseOnTable = true;
        }
    }
    check(livingOk && approx(scene.getRoom().getSettings().width, 6.0f) &&
          approx(scene.getRoom().getSettings().length, 7.0f) &&
          scene.getObjectCount() == 7 && vaseOnTable,
          "TC-M4-24 Minimalist Living Room preset valid (6x7x3m, 7 objects, vase on coffee table y=0.45)", failures);

    // 4. Benchmark scene builder (10 / 30 / 100 objects)
    const bool b10 = roomcraft::scene::ScenePresets::buildBenchmarkScene(10, scene, catalog, &err) &&
                     scene.getObjectCount() == 10;
    const bool b30 = roomcraft::scene::ScenePresets::buildBenchmarkScene(30, scene, catalog, &err) &&
                     scene.getObjectCount() == 30;
    const bool b100 = roomcraft::scene::ScenePresets::buildBenchmarkScene(100, scene, catalog, &err) &&
                      scene.getObjectCount() == 100;
    check(b10 && b30 && b100, "Benchmark scene generator builds 10 / 30 / 100 furniture instances cleanly", failures);

    return failures;
}
