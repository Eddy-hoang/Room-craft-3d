#include "roomcraft/assets/FurnitureCatalog.hpp"
#include "roomcraft/math/BoundingBox.hpp"
#include "roomcraft/math/OBB.hpp"
#include "roomcraft/math/SATIntersection.hpp"
#include "roomcraft/scene/Scene.hpp"
#include "roomcraft/scene/ScenePresets.hpp"
#include "roomcraft/systems/CollisionSystem.hpp"

#include <glm/geometric.hpp>

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>

using namespace roomcraft;

namespace {

int g_failures = 0;

void check(const char* name, bool condition)
{
    std::cout << "  [M8-Collision] " << name << ": " << (condition ? "PASS" : "FAIL") << std::endl;
    if (!condition) {
        ++g_failures;
    }
}

math::BoundingBox makeLocalBox(float w, float h, float d)
{
    math::BoundingBox b;
    b.min = glm::vec3(-w * 0.5f, 0.0f, -d * 0.5f);
    b.max = glm::vec3( w * 0.5f, h,     d * 0.5f);
    b.valid = true;
    return b;
}

bool nearEq(float a, float b, float eps = 1e-3f)
{
    return std::fabs(a - b) <= eps;
}

} // namespace

int main()
{
    std::cout << "================ RoomCraft 3D - M8 Collision Unit Tests (CPU) ================" << std::endl;

    // M8-T02: AABB overlap & separation
    {
        math::BoundingBox a;
        a.min = {0.0f, 0.0f, 0.0f}; a.max = {2.0f, 2.0f, 2.0f}; a.valid = true;
        math::BoundingBox b;
        b.min = {1.5f, 0.0f, 0.0f}; b.max = {3.5f, 2.0f, 2.0f}; b.valid = true;
        math::BoundingBox c;
        c.min = {2.5f, 0.0f, 0.0f}; c.max = {4.5f, 2.0f, 2.0f}; c.valid = true;

        const auto ov = math::intersectAABB(a, b, 1e-4f);
        const auto sep = math::intersectAABB(a, c, 1e-4f);
        const auto inv = math::intersectAABB(a, math::BoundingBox{}, 1e-4f);
        check("M8-T02 AABB overlap (depth=0.5m), separation & invalid box rejection",
              ov.isPenetrating() && nearEq(ov.minOverlap, 0.5f) &&
              sep.isSeparated() && sep.minOverlap < -0.4f && inv.isSeparated());
    }

    // M8-T03: AABB touching behavior (side-by-side flush & elevated lamp on desk y=0.75m)
    {
        math::BoundingBox desk;
        desk.min = {-0.7f, 0.0f, -0.35f}; desk.max = {0.7f, 0.75f, 0.35f}; desk.valid = true;
        math::BoundingBox lamp;
        lamp.min = {-0.15f, 0.75f, -0.15f}; lamp.max = {0.15f, 1.25f, 0.15f}; lamp.valid = true;
        math::BoundingBox adjacent;
        adjacent.min = {0.7f, 0.0f, -0.35f}; adjacent.max = {1.7f, 0.75f, 0.35f}; adjacent.valid = true;

        const auto touchTop = math::intersectAABB(desk, lamp, 0.005f);
        const auto touchSide = math::intersectAABB(desk, adjacent, 0.005f);
        check("M8-T03 AABB touching (lamp on desk y=0.75m & flush side contact) classified Touching, not Penetrating",
              touchTop.isTouching() && !touchTop.isPenetrating() &&
              touchSide.isTouching() && !touchSide.isPenetrating());
    }

    // M8-T04: 3D OBB SAT intersection & symmetry
    {
        const auto localA = makeLocalBox(2.0f, 1.0f, 0.6f);
        const auto localB = makeLocalBox(2.0f, 1.0f, 0.6f);

        const auto obbA = math::OBB::fromLocalBoundsAndTransform(
            localA, scene::Transform({0.0f, 0.0f, 0.0f}, {0.0f, 35.0f, 0.0f}, {1.0f, 1.0f, 1.0f}));
        const auto obbB = math::OBB::fromLocalBoundsAndTransform(
            localB, scene::Transform({0.3f, 0.0f, 0.2f}, {10.0f, -25.0f, 5.0f}, {1.0f, 1.0f, 1.0f}));

        const auto rAB = math::SATIntersection::testOBB(obbA, obbB, 0.001f);
        const auto rBA = math::SATIntersection::testOBB(obbB, obbA, 0.001f);
        check("M8-T04 3D OBB SAT penetrating detection & exact symmetry (A,B == B,A)",
              rAB.isPenetrating() && rBA.isPenetrating() &&
              nearEq(rAB.penetrationDepth, rBA.penetrationDepth, 1e-4f) &&
              rAB.penetrationDepth > 0.05f &&
              nearEq(glm::length(rAB.separationAxis), 1.0f, 1e-3f));
    }

    // M8-T05: Rotated OBB false-positive rejection (World AABBs overlap, OBB SAT separates cleanly!)
    {
        // Hai thanh dài mỏng xoay 45 độ song song nhau: World AABB phình lớn giao nhau rõ,
        // nhưng hai hộp OBB thực tế cách nhau rõ ràng theo trục pháp tuyến 45 độ.
        const auto plank = makeLocalBox(3.0f, 0.8f, 0.4f);
        const auto obb1 = math::OBB::fromLocalBoundsAndTransform(
            plank, scene::Transform({0.0f, 0.0f, 0.0f}, {0.0f, 45.0f, 0.0f}, {1.0f, 1.0f, 1.0f}));
        const auto obb2 = math::OBB::fromLocalBoundsAndTransform(
            plank, scene::Transform({0.65f, 0.0f, 0.65f}, {0.0f, 45.0f, 0.0f}, {1.0f, 1.0f, 1.0f}));

        const auto aabbRes = math::intersectAABB(obb1.toWorldAABB(), obb2.toWorldAABB(), 0.005f);
        bool passedBroad = false;
        const auto satRes = systems::CollisionSystem::testObjectPair(
            obb1.toWorldAABB(), obb1, obb2.toWorldAABB(), obb2, 0.005f, &passedBroad);

        check("M8-T05 Rotated 45-deg OBB rejects World-AABB false positive via SAT narrow-phase",
              aabbRes.isPenetrating() && passedBroad && satRes.isSeparated());
    }

    // M8-T06: Near-parallel SAT axes (0 deg, 0.0005 deg, 90 deg, 180 deg)
    {
        const auto box = makeLocalBox(1.0f, 1.0f, 1.0f);
        const auto obb0 = math::OBB::fromLocalBoundsAndTransform(box, scene::Transform({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
        const auto obbNear = math::OBB::fromLocalBoundsAndTransform(box, scene::Transform({0.5f, 0.0f, 0.0f}, {0.0f, 0.0005f, 0.0f}));
        const auto obb90 = math::OBB::fromLocalBoundsAndTransform(box, scene::Transform({1.0f, 0.0f, 0.0f}, {0.0f, 90.0f, 0.0f}));
        const auto obb180 = math::OBB::fromLocalBoundsAndTransform(box, scene::Transform({1.2f, 0.0f, 0.0f}, {0.0f, 180.0f, 0.0f}));

        const auto rNear = math::SATIntersection::testOBB(obb0, obbNear, 0.005f);
        const auto r90 = math::SATIntersection::testOBB(obb0, obb90, 0.005f);
        const auto r180 = math::SATIntersection::testOBB(obb0, obb180, 0.005f);

        check("M8-T06 Near-parallel SAT axes (0.0005/90/180 deg) finite & numerically stable",
              rNear.isPenetrating() && nearEq(rNear.penetrationDepth, 0.5f, 0.01f) &&
              std::isfinite(rNear.separationAxis.x) && std::isfinite(rNear.separationAxis.y) &&
              r90.isTouching() && r180.isSeparated());
    }

    // M8-T07: Non-uniform scale + rotation world bounds & singular/negative scale rejection
    {
        const auto local = makeLocalBox(1.0f, 2.0f, 0.5f);
        const scene::Transform tr({1.0f, 0.5f, -2.0f}, {15.0f, 60.0f, -10.0f}, {2.0f, 0.5f, 3.0f});
        const auto obb = math::OBB::fromLocalBoundsAndTransform(local, tr);
        const auto aabbFromObb = obb.toWorldAABB();
        const auto aabbFromMat = local.transformed(tr.getModelMatrix());

        const bool boundsMatch =
            obb.isValid() &&
            glm::length(aabbFromObb.min - aabbFromMat.min) < 1e-4f &&
            glm::length(aabbFromObb.max - aabbFromMat.max) < 1e-4f;

        const auto badZero = math::OBB::fromLocalBoundsAndTransform(local, scene::Transform({0, 0, 0}, {0, 0, 0}, {0.0f, 1.0f, 1.0f}));
        const auto badNeg = math::OBB::fromLocalBoundsAndTransform(local, scene::Transform({0, 0, 0}, {0, 0, 0}, {-1.0f, 1.0f, 1.0f}));
        const float nanV = std::numeric_limits<float>::quiet_NaN();
        const auto badNan = math::OBB::fromLocalBoundsAndTransform(local, scene::Transform({nanV, 0, 0}));

        check("M8-T07 Non-uniform scale OBB matches transformed AABB & rejects zero/negative/NaN transforms",
              boundsMatch && !badZero.isValid() && !badNeg.isValid() && !badNan.isValid());
    }

    // M8-T08: Room boundary validation (all 6 planes + rotated corner + valid elevated lamp on desk)
    {
        scene::Room room; // Default interior: X in [-2.5, 2.5], Y in [0, 3.0], Z in [-3.0, 3.0]
        const auto local = makeLocalBox(1.0f, 0.8f, 0.5f);

        // Elevated lamp at y=0.75m inside room -> valid
        const auto obbElev = math::OBB::fromLocalBoundsAndTransform(
            makeLocalBox(0.3f, 0.5f, 0.3f), scene::Transform({0.0f, 0.75f, 0.0f}));
        const auto rElev = systems::RoomBoundaryValidator::validateBounds(obbElev.toWorldAABB(), room, 0.005f);

        // Box near East wall: unrotated (halfX=0.25 at x=2.15 -> maxX=2.40 valid), rotated 90 deg (halfX=0.50 -> maxX=2.65 exceeds East!)
        const auto obbUnrot = math::OBB::fromLocalBoundsAndTransform(
            makeLocalBox(0.5f, 0.8f, 1.0f), scene::Transform({2.15f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
        const auto obbRot = math::OBB::fromLocalBoundsAndTransform(
            makeLocalBox(0.5f, 0.8f, 1.0f), scene::Transform({2.15f, 0.0f, 0.0f}, {0.0f, 90.0f, 0.0f}));
        const auto rUnrot = systems::RoomBoundaryValidator::validateBounds(obbUnrot.toWorldAABB(), room, 0.005f);
        const auto rRot = systems::RoomBoundaryValidator::validateBounds(obbRot.toWorldAABB(), room, 0.005f);

        // Below floor & above ceiling
        const auto rFloor = systems::RoomBoundaryValidator::validateBounds(
            math::OBB::fromLocalBoundsAndTransform(local, scene::Transform({0.0f, -0.2f, 0.0f})).toWorldAABB(), room, 0.005f);
        const auto rCeil = systems::RoomBoundaryValidator::validateBounds(
            math::OBB::fromLocalBoundsAndTransform(local, scene::Transform({0.0f, 2.5f, 0.0f})).toWorldAABB(), room, 0.005f);

        check("M8-T08 Room boundary checks rotated bounds, elevated placement, floor & ceiling accurately",
              rElev.valid && rUnrot.valid && !rRot.valid && rRot.exceedsEast &&
              !rFloor.valid && rFloor.belowFloor && !rCeil.valid && rCeil.aboveCeiling);
    }

    // M8-T09: Room resize keeps all furniture intact & revalidates bounds + hidden/rug policies
    {
        assets::FurnitureCatalog catalog;
        scene::Scene scene;
        const auto boundsLookup = [&catalog](const std::string& id) -> math::BoundingBox {
            const auto* e = catalog.findById(id);
            if (!e) return {};
            return makeLocalBox(e->defaultDimensions.x, e->defaultDimensions.y, e->defaultDimensions.z);
        };

        const auto idBed = scene.addObject("blender-modern-bed", "Bed", scene::Transform({-1.1f, 0.0f, -1.3f}), catalog);
        const auto idRug = scene.addObject("blender-carpet", "Rug", scene::Transform({-1.1f, 0.003f, -1.3f}), catalog);
        const auto idHidden = scene.addObject("blender-modern-sofa", "Hidden Sofa", scene::Transform({-1.1f, 0.0f, -1.3f}), catalog);
        scene.setObjectVisibility(idHidden, false);

        systems::CollisionOptions opts;
        opts.allowRugOverlap = true;
        opts.includeHiddenObjects = false;
        const auto repBefore = systems::CollisionSystem::validateScene(scene, boundsLookup, opts);

        // Thu nhỏ phòng xuống 3x3x3m -> không được tự xóa hay tự dịch chuyển đồ nội thất, nhưng phải báo vượt biên!
        scene::RoomSettings smallRoom = scene.getRoom().getSettings();
        smallRoom.width = 3.0f;
        smallRoom.length = 3.0f;
        scene.getRoom().applySettings(smallRoom);
        const auto repAfterResize = systems::CollisionSystem::validateScene(scene, boundsLookup, opts);

        // Nếu bật includeHiddenObjects=true -> phát hiện va chạm với Hidden Sofa
        opts.includeHiddenObjects = true;
        const auto repWithHidden = systems::CollisionSystem::validateScene(scene, boundsLookup, opts);

        const auto* bedPtr = scene.findObject(idBed);
        check("M8-T09 Room resize preserves furniture & revalidates bounds; hidden & rug policies verified",
              repBefore.collisions.empty() && repBefore.outOfBoundsObjectIds.empty() &&
              scene.getObjectCount() == 3 && bedPtr != nullptr &&
              nearEq(bedPtr->transform.position.x, -1.1f) &&
              !repAfterResize.outOfBoundsObjectIds.empty() &&
              !repWithHidden.collisions.empty() &&
              scene::isValidObjectId(idRug));
    }

    // M8 Performance Benchmark (Section 15): 30, 100 và 250 vật thể + incremental O(N) check
    {
        assets::FurnitureCatalog catalog;
        const auto boundsLookup = [&catalog](const std::string& id) -> math::BoundingBox {
            const auto* e = catalog.findById(id);
            if (!e) return {};
            return makeLocalBox(e->defaultDimensions.x, e->defaultDimensions.y, e->defaultDimensions.z);
        };

        auto runBench = [&](std::size_t count) -> bool {
            scene::Scene s;
            scene::ScenePresets::buildBenchmarkScene(count, s, catalog);
            // Warm-up 2 runs + measure 20 runs
            systems::CollisionOptions opts;
            for (int i = 0; i < 2; ++i) {
                (void)systems::CollisionSystem::validateScene(s, boundsLookup, opts);
            }
            double totalMs = 0.0;
            systems::SceneCollisionReport last;
            constexpr int kRuns = 20;
            for (int i = 0; i < kRuns; ++i) {
                last = systems::CollisionSystem::validateScene(s, boundsLookup, opts);
                totalMs += last.elapsedMs;
            }
            const double avgMs = totalMs / static_cast<double>(kRuns);

            // Đo kiểm tra tăng dần 1 vật thể O(N) khi đang kéo chuột
            const auto t0 = std::chrono::steady_clock::now();
            const auto& firstObj = s.getObjects().front();
            for (int i = 0; i < 100; ++i) {
                (void)systems::CollisionSystem::validateCandidateObject(
                    s, firstObj.id, firstObj.assetId, firstObj.transform, boundsLookup, opts);
            }
            const auto t1 = std::chrono::steady_clock::now();
            const double avgSingleMs = std::chrono::duration<double, std::milli>(t1 - t0).count() / 100.0;

            std::cout << "  [M8-Bench] N=" << count
                      << " | broadPairs=" << last.broadPhasePairsTested
                      << " | narrowSAT=" << last.narrowPhaseSatTests
                      << " | collisions=" << last.collisions.size()
                      << " | fullAvgMs=" << std::fixed << std::setprecision(4) << avgMs
                      << " | incremental1ObjMs=" << avgSingleMs << std::endl;
            return last.activeObjects == count && avgMs < 50.0;
        };

        const bool b30 = runBench(30);
        const bool b100 = runBench(100);
        const bool b250 = runBench(250);
        check("M8-PERF Collision benchmark (30 / 100 / 250 objects + incremental single-object check)", b30 && b100 && b250);
    }

    // RC2.3 P0 Regression: Toàn bộ 3 phòng mẫu mặc định phải đạt 0 va chạm và 0 vượt biên (BUG-04)
    {
        assets::FurnitureCatalog catalog;
        const auto boundsLookup = [&catalog](const std::string& id) -> math::BoundingBox {
            const auto* e = catalog.findById(id);
            if (!e) return {};
            return makeLocalBox(e->defaultDimensions.x, e->defaultDimensions.y, e->defaultDimensions.z);
        };
        const scene::ScenePresetType presets[3] = {
            scene::ScenePresetType::ModernBedroom,
            scene::ScenePresetType::GamingWorkspace,
            scene::ScenePresetType::MinimalistLivingRoom
        };
        bool allPresetsClean = true;
        for (const auto p : presets) {
            scene::Scene s;
            std::string err;
            const bool loaded = scene::ScenePresets::applyPreset(p, s, catalog, &err);
            systems::CollisionOptions opts;
            const auto rep = systems::CollisionSystem::validateScene(s, boundsLookup, opts);
            if (!loaded || !rep.collisions.empty() || !rep.outOfBoundsObjectIds.empty()) {
                allPresetsClean = false;
                std::cout << "    [FAIL] Preset " << static_cast<int>(p)
                          << " collisions=" << rep.collisions.size()
                          << " outOfBounds=" << rep.outOfBoundsObjectIds.size() << std::endl;
            }
        }
        check("RC2-P0-PRESET All 3 default room presets pass collision & room boundary validation (0 warnings)",
              allPresetsClean);
    }

    std::cout << "==============================================================================" << std::endl;
    if (g_failures == 0) {
        std::cout << "[M8-COLLISION-TESTS] ALL PASSED (0 failures)." << std::endl;
        return 0;
    }
    std::cout << "[M8-COLLISION-TESTS] FAILED (" << g_failures << " failures)." << std::endl;
    return 1;
}
