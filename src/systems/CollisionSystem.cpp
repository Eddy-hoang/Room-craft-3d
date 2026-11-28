#include "roomcraft/systems/CollisionSystem.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <sstream>

namespace roomcraft::systems {

std::string RoomBoundaryResult::formatSummary() const
{
    if (valid) {
        return "Within room bounds";
    }
    std::vector<std::string> sides;
    if (exceedsWest) sides.emplace_back("West (-X)");
    if (exceedsEast) sides.emplace_back("East (+X)");
    if (exceedsNorth) sides.emplace_back("North (-Z)");
    if (exceedsSouth) sides.emplace_back("South (+Z)");
    if (belowFloor) sides.emplace_back("Floor (-Y)");
    if (aboveCeiling) sides.emplace_back("Ceiling (+Y)");

    std::ostringstream oss;
    oss << "Outside room: ";
    for (std::size_t i = 0; i < sides.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << sides[i];
    }
    return oss.str();
}

std::size_t SceneCollisionReport::getAffectedObjectCount() const
{
    std::size_t count = 0;
    for (const auto& kv : objectStatuses) {
        if (kv.second.hasAnyIssue()) {
            ++count;
        }
    }
    return count;
}

const ObjectCollisionStatus* SceneCollisionReport::findStatus(scene::ObjectId id) const
{
    const auto it = objectStatuses.find(id);
    return (it != objectStatuses.end()) ? &it->second : nullptr;
}

RoomBoundaryResult RoomBoundaryValidator::validateBounds(const math::BoundingBox& worldBounds,
                                                         const scene::Room& room,
                                                         float tolerance)
{
    RoomBoundaryResult res;
    if (!worldBounds.isValid()) {
        res.valid = true;
        return res;
    }

    const float tol = std::max(0.0f, tolerance);
    const math::BoundingBox interior = room.getInteriorBounds();

    const float westOver = interior.min.x - worldBounds.min.x;
    const float eastOver = worldBounds.max.x - interior.max.x;
    const float northOver = interior.min.z - worldBounds.min.z;
    const float southOver = worldBounds.max.z - interior.max.z;
    const float floorOver = interior.min.y - worldBounds.min.y;
    const float ceilOver = worldBounds.max.y - interior.max.y;

    res.exceedsWest = westOver > tol;
    res.exceedsEast = eastOver > tol;
    res.exceedsNorth = northOver > tol;
    res.exceedsSouth = southOver > tol;
    res.belowFloor = floorOver > tol;
    res.aboveCeiling = ceilOver > tol;

    res.maxViolationDepth = std::max({
        0.0f,
        res.exceedsWest ? westOver : 0.0f,
        res.exceedsEast ? eastOver : 0.0f,
        res.exceedsNorth ? northOver : 0.0f,
        res.exceedsSouth ? southOver : 0.0f,
        res.belowFloor ? floorOver : 0.0f,
        res.aboveCeiling ? ceilOver : 0.0f
    });

    res.valid = !(res.exceedsWest || res.exceedsEast ||
                  res.exceedsNorth || res.exceedsSouth ||
                  res.belowFloor || res.aboveCeiling);
    return res;
}

bool CollisionSystem::isFloorCovering(const scene::SceneObject& obj, const math::BoundingBox& worldAabb)
{
    if (obj.assetId == "blender-carpet") {
        return true;
    }
    if (!worldAabb.isValid()) {
        return false;
    }
    const glm::vec3 size = worldAabb.getSize();
    // Vật phẳng sát sàn cao <= 4cm và đặt sát mặt sàn (y <= 2cm)
    return (size.y <= 0.04f) && (worldAabb.min.y >= -0.01f) && (worldAabb.min.y <= 0.02f);
}

math::SATResult CollisionSystem::testObjectPair(const math::BoundingBox& aabbA,
                                                const math::OBB& obbA,
                                                const math::BoundingBox& aabbB,
                                                const math::OBB& obbB,
                                                float penetrationTolerance,
                                                bool* outPassedBroadPhase)
{
    // Giai đoạn 1: Broad-Phase AABB nhanh
    const auto aabbRes = math::intersectAABB(aabbA, aabbB, penetrationTolerance);
    if (!aabbRes.isPenetrating()) {
        if (outPassedBroadPhase != nullptr) *outPassedBroadPhase = false;
        math::SATResult quick;
        quick.state = aabbRes.state;
        quick.penetrationDepth = aabbRes.minOverlap;
        return quick;
    }

    if (outPassedBroadPhase != nullptr) *outPassedBroadPhase = true;

    // Giai đoạn 2: Narrow-Phase OBB SAT (15 trục)
    return math::SATIntersection::testOBB(obbA, obbB, penetrationTolerance);
}

SceneCollisionReport CollisionSystem::validateScene(const scene::Scene& scene,
                                                    const LocalBoundsLookupFn& localBoundsLookup,
                                                    const CollisionOptions& options)
{
    const auto t0 = std::chrono::steady_clock::now();
    SceneCollisionReport report;

    struct ActiveEntry {
        const scene::SceneObject* obj = nullptr;
        math::BoundingBox worldAabb;
        math::OBB worldObb;
        bool isRug = false;
    };

    const auto& objects = scene.getObjects();
    std::vector<ActiveEntry> active;
    active.reserve(objects.size());

    for (const auto& obj : objects) {
        if (!options.includeHiddenObjects && !obj.visible) {
            continue;
        }
        const math::BoundingBox localBounds = localBoundsLookup ? localBoundsLookup(obj.assetId) : math::BoundingBox{};
        if (!localBounds.isValid()) {
            continue;
        }
        const math::OBB obb = math::OBB::fromLocalBoundsAndTransform(localBounds, obj.transform);
        if (!obb.isValid()) {
            continue;
        }
        const math::BoundingBox worldAabb = obb.toWorldAABB();

        ObjectCollisionStatus status;
        status.objectId = obj.id;
        status.worldAabb = worldAabb;
        status.worldObb = obb;
        status.boundsValid = true;
        status.boundary = RoomBoundaryValidator::validateBounds(worldAabb, scene.getRoom(), options.boundaryTolerance);
        if (!status.boundary.valid) {
            report.outOfBoundsObjectIds.push_back(obj.id);
        }
        report.objectStatuses[obj.id] = std::move(status);

        active.push_back(ActiveEntry{
            &obj,
            worldAabb,
            obb,
            options.allowRugOverlap && isFloorCovering(obj, worldAabb)
        });
    }

    report.activeObjects = active.size();

    // Kiểm tra từng cặp duy nhất (i < j): không tự va chạm, không trùng lặp cặp
    for (std::size_t i = 0; i < active.size(); ++i) {
        for (std::size_t j = i + 1; j < active.size(); ++j) {
            const auto& a = active[i];
            const auto& b = active[j];

            // Bỏ qua cặp có đúng một bên là thảm trải sàn (cho phép đặt ghế/bàn/giường trên thảm)
            if (options.allowRugOverlap && (a.isRug != b.isRug)) {
                continue;
            }

            ++report.broadPhasePairsTested;
            bool passedBroad = false;
            const auto sat = testObjectPair(a.worldAabb, a.worldObb,
                                            b.worldAabb, b.worldObb,
                                            options.penetrationTolerance,
                                            &passedBroad);
            if (passedBroad) {
                ++report.broadPhaseOverlaps;
                ++report.narrowPhaseSatTests;
            }

            if (sat.isPenetrating()) {
                CollisionPair pair;
                const bool swapOrder = (a.obj->id > b.obj->id);
                pair.objectA = swapOrder ? b.obj->id : a.obj->id;
                pair.objectB = swapOrder ? a.obj->id : b.obj->id;
                pair.nameA = swapOrder ? b.obj->name : a.obj->name;
                pair.nameB = swapOrder ? a.obj->name : b.obj->name;
                pair.contactState = sat.state;
                pair.penetrationDepth = sat.penetrationDepth;
                pair.separationAxis = swapOrder ? -sat.separationAxis : sat.separationAxis;
                report.collisions.push_back(std::move(pair));

                auto& stA = report.objectStatuses[a.obj->id];
                stA.collidingObjectIds.push_back(b.obj->id);
                stA.collidingObjectNames.push_back(b.obj->name);

                auto& stB = report.objectStatuses[b.obj->id];
                stB.collidingObjectIds.push_back(a.obj->id);
                stB.collidingObjectNames.push_back(a.obj->name);
            }
        }
    }

    const auto t1 = std::chrono::steady_clock::now();
    report.elapsedMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return report;
}

ObjectCollisionStatus CollisionSystem::validateCandidateObject(const scene::Scene& scene,
                                                               scene::ObjectId objectId,
                                                               const std::string& assetId,
                                                               const scene::Transform& candidateTransform,
                                                               const LocalBoundsLookupFn& localBoundsLookup,
                                                               const CollisionOptions& options)
{
    ObjectCollisionStatus status;
    status.objectId = objectId;

    const math::BoundingBox localBounds = localBoundsLookup ? localBoundsLookup(assetId) : math::BoundingBox{};
    if (!localBounds.isValid()) {
        return status;
    }

    const math::OBB candObb = math::OBB::fromLocalBoundsAndTransform(localBounds, candidateTransform);
    if (!candObb.isValid()) {
        return status;
    }

    const math::BoundingBox candAabb = candObb.toWorldAABB();
    status.worldAabb = candAabb;
    status.worldObb = candObb;
    status.boundsValid = true;
    status.boundary = RoomBoundaryValidator::validateBounds(candAabb, scene.getRoom(), options.boundaryTolerance);

    scene::SceneObject tempObj;
    tempObj.id = objectId;
    tempObj.assetId = assetId;
    const bool candIsRug = options.allowRugOverlap && isFloorCovering(tempObj, candAabb);

    for (const auto& other : scene.getObjects()) {
        if (other.id == objectId) {
            continue;
        }
        if (!options.includeHiddenObjects && !other.visible) {
            continue;
        }
        const math::BoundingBox otherLocal = localBoundsLookup ? localBoundsLookup(other.assetId) : math::BoundingBox{};
        if (!otherLocal.isValid()) {
            continue;
        }
        const math::OBB otherObb = math::OBB::fromLocalBoundsAndTransform(otherLocal, other.transform);
        if (!otherObb.isValid()) {
            continue;
        }
        const math::BoundingBox otherAabb = otherObb.toWorldAABB();
        const bool otherIsRug = options.allowRugOverlap && isFloorCovering(other, otherAabb);
        if (options.allowRugOverlap && (candIsRug != otherIsRug)) {
            continue;
        }

        const auto sat = testObjectPair(candAabb, candObb, otherAabb, otherObb, options.penetrationTolerance);
        if (sat.isPenetrating()) {
            status.collidingObjectIds.push_back(other.id);
            status.collidingObjectNames.push_back(other.name);
        }
    }

    return status;
}

} // namespace roomcraft::systems
