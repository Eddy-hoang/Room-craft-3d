#include "roomcraft/assets/AssetData.hpp"
#include "roomcraft/assets/FurnitureCatalog.hpp"
#include "roomcraft/geometry/FurnitureGenerator.hpp"
#include "roomcraft/scene/Scene.hpp"
#include "roomcraft/systems/PickingSystem.hpp"

#include <cmath>
#include <iostream>
#include <unordered_map>

namespace {

bool check(bool cond, const char* label, int& failures)
{
    std::cout << "  [Picking] " << label << ": " << (cond ? "PASS" : "FAIL") << std::endl;
    if (!cond) ++failures;
    return cond;
}

} // namespace

int runPickingTests()
{
    int failures = 0;

    // Xây dựng CPU AssetData thật từ Procedural FurnitureGenerator (không cần GPU/Assimp)
    roomcraft::assets::FurnitureCatalog catalog;
    std::unordered_map<std::string, roomcraft::assets::AssetData> cpuAssets;

    roomcraft::geometry::FurnitureModel deskModel;
    roomcraft::geometry::FurnitureGenerator::createDesk(roomcraft::geometry::DeskParams{}, deskModel);
    cpuAssets["procedural-desk"] = roomcraft::assets::AssetData::fromFurnitureModel(
        "procedural-desk", roomcraft::assets::AssetCategory::Tables, deskModel);

    roomcraft::geometry::FurnitureModel shelfModel;
    roomcraft::geometry::FurnitureGenerator::createBookshelf(roomcraft::geometry::BookshelfParams{}, shelfModel);
    cpuAssets["procedural-bookshelf"] = roomcraft::assets::AssetData::fromFurnitureModel(
        "procedural-bookshelf", roomcraft::assets::AssetCategory::Storage, shelfModel);

    const auto lookup = [&cpuAssets](const std::string& id) -> const roomcraft::assets::AssetData* {
        const auto it = cpuAssets.find(id);
        return (it != cpuAssets.end()) ? &it->second : nullptr;
    };

    roomcraft::scene::Scene scene;
    const auto frontId = scene.addObject("procedural-bookshelf", "Front Shelf",
                                         roomcraft::scene::Transform({0.0f, 0.0f, 1.0f}), catalog);
    const auto backId = scene.addObject("procedural-bookshelf", "Back Shelf",
                                        roomcraft::scene::Transform({0.0f, 0.0f, -1.0f}), catalog);

    // TC-M5-14: Nearest object selection
    roomcraft::math::Ray straightRay;
    roomcraft::math::Ray::create({0.0f, 0.9f, 2.4f}, {0.0f, 0.0f, -1.0f}, straightRay);
    const auto pickNearest = roomcraft::systems::PickingSystem::pickObject(straightRay, scene, lookup);
    check(pickNearest.hit && pickNearest.objectId == frontId && pickNearest.usedTriangleRefinement,
          "TC-M5-14 Picking selects nearest visible object along ray", failures);

    // TC-M5-15: Hidden object exclusion -> ray passes through hidden front shelf and hits back shelf
    scene.setObjectVisibility(frontId, false);
    const auto pickAfterHide = roomcraft::systems::PickingSystem::pickObject(straightRay, scene, lookup);
    check(pickAfterHide.hit && pickAfterHide.objectId == backId,
          "TC-M5-15 Hidden object excluded; ray picks next visible object behind it", failures);
    scene.setObjectVisibility(frontId, true);

    // TC-M5-17: Empty space returns no hit
    roomcraft::math::Ray missRay;
    roomcraft::math::Ray::create({2.0f, 1.5f, 2.4f}, {0.0f, 0.0f, -1.0f}, missRay);
    const auto pickEmpty = roomcraft::systems::PickingSystem::pickObject(missRay, scene, lookup);
    check(!pickEmpty.hit && pickEmpty.objectId == roomcraft::scene::kInvalidObjectId,
          "TC-M5-17 Ray into empty space returns no hit", failures);

    // TC-M5-18: Rotated & scaled object bounds picking
    scene.clearObjects();
    const auto rotDeskId = scene.addObject("procedural-desk", "Rotated Desk",
                                           roomcraft::scene::Transform({0.0f, 0.0f, 0.0f}, {0.0f, 90.0f, 0.0f}, {1.5f, 1.0f, 1.0f}),
                                           catalog);
    // Bàn xoay 90° Y & scale X 1.5 -> dài 2.1m dọc trục Z ([-1.05, 1.05]), điểm (0, 0.72, 0.90) thuộc mặt bàn
    roomcraft::math::Ray rotHitRay;
    roomcraft::math::Ray::create({0.0f, 2.0f, 0.90f}, {0.0f, -1.0f, 0.0f}, rotHitRay);
    const auto pickRot = roomcraft::systems::PickingSystem::pickObject(rotHitRay, scene, lookup);
    check(pickRot.hit && pickRot.objectId == rotDeskId,
          "TC-M5-18 Picking accurately hits rotated and non-uniformly scaled object", failures);

    // Ray-Triangle Narrow-phase Refinement: tia đi qua khoảng trống dưới gầm bàn (y=0.30, x=0) -> AABB trúng nhưng Triangle KHÔNG trúng!
    scene.clearObjects();
    const auto deskId = scene.addObject("procedural-desk", "Hollow Desk",
                                        roomcraft::scene::Transform({0.0f, 0.0f, 0.0f}), catalog);
    roomcraft::math::Ray gapRay;
    roomcraft::math::Ray::create({0.0f, 0.30f, 2.0f}, {0.0f, 0.0f, -1.0f}, gapRay);
    roomcraft::systems::PickOptions aabbOnly{false, false};
    roomcraft::systems::PickOptions triRefined{true, false};
    const auto hitAabb = roomcraft::systems::PickingSystem::pickObject(gapRay, scene, lookup, aabbOnly);
    const auto hitTri = roomcraft::systems::PickingSystem::pickObject(gapRay, scene, lookup, triRefined);
    check(hitAabb.hit && hitAabb.objectId == deskId && !hitTri.hit,
          "Ray-Triangle refinement passes cleanly through hollow space under desk (unlike AABB-only)", failures);

    // Occlusion-Aware Wall Policy:
    // Camera đứng bên ngoài tường Nam (+Z = 5.0m) bắn về phía bàn ở tâm phòng:
    // - Khi WallVisibilityMode = CutawayView (tường Nam ẩn): tia xuyên qua chọn đúng bàn.
    // - Khi WallVisibilityMode = AllWalls (tường Nam hiện): tường Nam chặn picking (occludedByWall=true).
    roomcraft::math::Ray outsideRay;
    roomcraft::math::Ray::create({0.0f, 0.72f, 5.0f}, {0.0f, 0.0f, -1.0f}, outsideRay);

    auto rs = scene.getRoom().getSettings();
    rs.wallVisibilityMode = roomcraft::scene::WallVisibilityMode::CutawayView;
    scene.getRoom().applySettings(rs);
    const auto pickCutaway = roomcraft::systems::PickingSystem::pickObject(outsideRay, scene, lookup);

    rs.wallVisibilityMode = roomcraft::scene::WallVisibilityMode::AllWalls;
    scene.getRoom().applySettings(rs);
    const auto pickOccluded = roomcraft::systems::PickingSystem::pickObject(outsideRay, scene, lookup);
    check(pickCutaway.hit && pickCutaway.objectId == deskId && !pickOccluded.hit && pickOccluded.occludedByWall,
          "Occlusion-aware picking: hidden cutaway wall allows picking, visible wall blocks picking", failures);

    return failures;
}
