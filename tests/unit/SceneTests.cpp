#include "roomcraft/assets/FurnitureCatalog.hpp"
#include "roomcraft/scene/Scene.hpp"
#include "roomcraft/systems/SelectionState.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <string>

namespace {

bool approx(float a, float b, float eps = 1e-4f)
{
    return std::abs(a - b) <= eps;
}

bool check(bool cond, const char* label, int& failures)
{
    std::cout << "  [Scene] " << label << ": " << (cond ? "PASS" : "FAIL") << std::endl;
    if (!cond) ++failures;
    return cond;
}

} // namespace

int runSceneTests()
{
    int failures = 0;
    roomcraft::assets::FurnitureCatalog catalog;
    roomcraft::scene::Scene scene;
    roomcraft::systems::SelectionState selection;
    std::string err;

    // TC-M4-09: Add procedural object
    const auto deskId = scene.addObject("procedural-desk", "My Desk",
                                        roomcraft::scene::Transform({0.0f, 0.0f, 0.0f}), catalog, &err);
    check(roomcraft::scene::isValidObjectId(deskId) && scene.getObjectCount() == 1 &&
          scene.findObject(deskId) != nullptr && scene.findObject(deskId)->assetId == "procedural-desk",
          "TC-M4-09 Add procedural object", failures);

    // TC-M4-10: Add imported Blender object
    const auto chair1 = scene.addObject("blender-gaming-chair", "Chair A",
                                        roomcraft::scene::Transform({1.0f, 0.0f, 0.0f}), catalog, &err);
    check(roomcraft::scene::isValidObjectId(chair1) && scene.getObjectCount() == 2 &&
          scene.findObject(chair1)->assetId == "blender-gaming-chair",
          "TC-M4-10 Add imported Blender object", failures);

    // TC-M4-11 & TC-M4-13: Duplicate creates new unique ID; deleting does not change remaining IDs or reuse deleted ID
    const auto chair2 = scene.duplicateObject(chair1, glm::vec3(0.5f, 0.0f, 0.5f), &err);
    const auto chair3 = scene.duplicateObject(chair1, glm::vec3(-0.5f, 0.0f, 0.5f), &err);
    const bool uniqueIds = (deskId != chair1) && (chair1 != chair2) && (chair2 != chair3) && (chair1 != chair3);
    check(uniqueIds && approx(scene.findObject(chair2)->transform.position.x, 1.5f),
          "TC-M4-11/13 Unique stable ObjectIds & duplicate offset", failures);

    scene.findObject(chair2)->shininessScale = 2.5f;
    check(approx(scene.findObject(chair1)->shininessScale, 1.0f) &&
          approx(scene.findObject(chair3)->shininessScale, 1.0f),
          "M7 shared chairs keep independent material adjustments", failures);

    // TC-M4-16: Transform independence across 3 instances of same Asset ID
    auto tr1 = scene.findObject(chair1)->transform;
    tr1.rotationDegrees.y = 90.0f;
    tr1.scale = glm::vec3(1.2f, 1.2f, 1.2f);
    scene.setObjectTransform(chair1, tr1, &err);
    const bool indepOk = approx(scene.findObject(chair1)->transform.rotationDegrees.y, 90.0f) &&
                         approx(scene.findObject(chair2)->transform.rotationDegrees.y, 0.0f) &&
                         approx(scene.findObject(chair3)->transform.scale.x, 1.0f);
    check(indepOk, "TC-M4-16 Transform independence across multiple instances of same asset", failures);

    // TC-M4-14: Rename object & reject empty/whitespace names
    const bool renameOk = scene.renameObject(chair1, "  Leader Chair  ", &err) &&
                          scene.findObject(chair1)->name == "Leader Chair" &&
                          !scene.renameObject(chair1, "   ", &err) &&
                          !scene.renameObject(999999, "Ghost", &err);
    check(renameOk, "TC-M4-14 Rename object trims whitespace and rejects empty/invalid IDs", failures);

    // TC-M4-15: Visibility toggle
    scene.setObjectVisibility(chair2, false);
    check(!scene.findObject(chair2)->visible && scene.getVisibleObjectCount() == 3 && scene.getObjectCount() == 4,
          "TC-M4-15 Object visibility toggle updates visible count without removing object", failures);

    // TC-M4-17: Invalid transform rejection (zero/negative/NaN scale, NaN position)
    roomcraft::scene::Transform badZeroScale({0, 0, 0}, {0, 0, 0}, {0.0f, 1.0f, 1.0f});
    roomcraft::scene::Transform badNegScale({0, 0, 0}, {0, 0, 0}, {1.0f, -1.0f, 1.0f});
    roomcraft::scene::Transform badNanPos({std::numeric_limits<float>::quiet_NaN(), 0, 0});
    const bool badTrRejected = !scene.setObjectTransform(chair1, badZeroScale, &err) &&
                               !scene.setObjectTransform(chair1, badNegScale, &err) &&
                               !scene.setObjectTransform(chair1, badNanPos, &err);
    check(badTrRejected, "TC-M4-17 Invalid transforms (0/neg scale, NaN) rejected cleanly", failures);

    // TC-M4-18: Invalid Asset ID rejection
    const auto badObj = scene.addObject("nonexistent-asset-xyz", "Bad", roomcraft::scene::Transform(), catalog, &err);
    check(!roomcraft::scene::isValidObjectId(badObj) && scene.getObjectCount() == 4,
          "TC-M4-18 Invalid Asset ID rejected without modifying scene", failures);

    // TC-M4-12 & TC-M4-19 & TC-M4-20: SelectionState + Remove selected object safely + no ID reuse
    selection.selectObject(chair2);
    const bool selBefore = selection.hasSelection() && selection.isSelected(chair2);
    const bool removed = scene.removeObject(chair2) && !scene.removeObject(chair2) && !scene.removeObject(999999);
    selection.reconcileWithScene(scene);
    const bool selAfter = !selection.hasSelection() && (scene.findObject(chair1) != nullptr) &&
                          (scene.findObject(chair3) != nullptr);
    const auto chair4 = scene.addObject("blender-gaming-chair", "", roomcraft::scene::Transform(), catalog, &err);
    check(selBefore && removed && selAfter && (chair4 != chair2) && (chair4 > chair3),
          "TC-M4-12/19/20 SelectionState, safe removal of selected object & monotonic non-reused ID", failures);

    return failures;
}
