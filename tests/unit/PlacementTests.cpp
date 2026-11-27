#include "roomcraft/assets/FurnitureCatalog.hpp"
#include "roomcraft/graphics/Camera.hpp"
#include "roomcraft/scene/Scene.hpp"
#include "roomcraft/systems/PlacementSystem.hpp"
#include "roomcraft/systems/SelectionState.hpp"

#include <cmath>
#include <iostream>
#include <string>

namespace {

bool approx(float a, float b, float eps = 1e-4f)
{
    return std::abs(a - b) <= eps;
}

bool check(bool cond, const char* label, int& failures)
{
    std::cout << "  [Placement] " << label << ": " << (cond ? "PASS" : "FAIL") << std::endl;
    if (!cond) ++failures;
    return cond;
}

} // namespace

int runPlacementTests()
{
    int failures = 0;
    roomcraft::assets::FurnitureCatalog catalog;
    roomcraft::scene::Scene scene;
    roomcraft::systems::SelectionState selection;
    std::string err;

    // Grid snapping cơ bản (M4-T19): snap X/Z theo 0.5m, giữ nguyên Y (để đặt vật lên bàn)
    const glm::vec3 snapped = roomcraft::systems::PlacementSystem::snapToGrid(glm::vec3(1.23f, 0.75f, -0.78f), 0.5f, false);
    check(approx(snapped.x, 1.0f) && approx(snapped.y, 0.75f) && approx(snapped.z, -1.0f),
          "Grid snapping rounds X/Z to gridSpacing and preserves Y elevation", failures);

    // PlacementSystem đặt đối tượng đầu tại tâm, đối tượng tiếp theo lệch vị trí và chọn đối tượng mới
    const auto id1 = roomcraft::systems::PlacementSystem::placeFurniture(
        scene, "procedural-desk", catalog, &selection, true, &err);
    const auto id2 = roomcraft::systems::PlacementSystem::placeFurniture(
        scene, "blender-gaming-chair", catalog, &selection, true, &err);
    const bool placedOk = roomcraft::scene::isValidObjectId(id1) &&
                          roomcraft::scene::isValidObjectId(id2) &&
                          selection.isSelected(id2) &&
                          approx(scene.findObject(id1)->transform.position.x, 0.0f) &&
                          approx(scene.findObject(id1)->transform.position.z, 0.0f) &&
                          (glm::length(scene.findObject(id2)->transform.position -
                                       scene.findObject(id1)->transform.position) > 0.1f);
    check(placedOk, "PlacementSystem places furniture inside room & selects newly added object", failures);

    // TC-M4-25: Room Camera Fit (Perspective & Orthographic)
    roomcraft::graphics::Camera cam;
    cam.setAspectRatio(16.0f / 9.0f);
    cam.setProjectionMode(roomcraft::graphics::ProjectionMode::Perspective);
    cam.lookAt({5.5f, 4.8f, 6.2f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
    const bool fitPersp = cam.fitBoundingBox(scene.getRoom().getExteriorBounds(), 1.22f);

    cam.setProjectionMode(roomcraft::graphics::ProjectionMode::Orthographic);
    cam.lookAt({0.0f, 12.0f, 0.001f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f});
    const bool fitOrtho = cam.fitBoundingBox(scene.getRoom().getExteriorBounds(), 1.22f);

    check(fitPersp && fitOrtho && (cam.getOrthoHalfHeight() > 2.5f),
          "TC-M4-25 Room Camera Fit works for both Perspective and Top Orthographic", failures);

    return failures;
}
