#include "roomcraft/systems/PickingSystem.hpp"

#include "roomcraft/math/Intersection.hpp"

#include <limits>

namespace roomcraft::systems {

float PickingSystem::computeNearestVisibleWallHit(const math::Ray& ray, const scene::Room& room)
{
    const auto& rs = room.getSettings();
    float nearestT = std::numeric_limits<float>::infinity();

    auto testMeshBox = [&](bool visible, const geometry::MeshData& mesh) {
        if (!visible || !mesh.boundingBox.isValid()) {
            return;
        }
        math::RayAabbHit wallHit;
        // Bỏ qua trường hợp camera nằm ngay bên trong độ dày tường (startedInside) để tránh chặn nhầm
        if (math::intersectRayAABB(ray, mesh.boundingBox, wallHit) && !wallHit.startedInside) {
            if (wallHit.tHit < nearestT) {
                nearestT = wallHit.tHit;
            }
        }
    };

    testMeshBox(rs.isNorthWallVisible(), room.getNorthWallMesh());
    testMeshBox(rs.isSouthWallVisible(), room.getSouthWallMesh());
    testMeshBox(rs.isEastWallVisible(), room.getEastWallMesh());
    testMeshBox(rs.isWestWallVisible(), room.getWestWallMesh());
    testMeshBox(rs.isCeilingVisible(), room.getCeilingMesh());
    return nearestT;
}

bool PickingSystem::intersectAssetTriangles(const math::Ray& ray,
                                            const scene::SceneObject& object,
                                            const assets::AssetData& asset,
                                            float& outNearestT,
                                            glm::vec3& outHitPoint)
{
    bool anyHit = false;
    float bestT = std::numeric_limits<float>::infinity();
    glm::vec3 bestPt(0.0f);

    const glm::mat4 objectMatrix = object.getModelMatrix();

    for (const auto& sub : asset.subMeshes) {
        if (sub.meshData.topology != graphics::PrimitiveTopology::Triangles ||
            sub.meshData.indices.size() < 3) {
            continue;
        }

        // Kết hợp đúng 1 lần ma trận đối tượng và ma trận cục bộ của sub-mesh
        const glm::mat4 worldPartMatrix = objectMatrix * sub.localTransform.getModelMatrix();

        // Lọc nhanh theo AABB thế giới của riêng từng sub-mesh trước khi duyệt tam giác
        const math::BoundingBox subWorldBox = sub.meshData.boundingBox.transformed(worldPartMatrix);
        math::RayAabbHit subBoxHit;
        if (!math::intersectRayAABB(ray, subWorldBox, subBoxHit) || subBoxHit.tHit >= bestT) {
            continue;
        }

        const auto& verts = sub.meshData.vertices;
        const auto& indices = sub.meshData.indices;
        for (std::size_t i = 0; i + 2 < indices.size(); i += 3) {
            const std::uint32_t i0 = indices[i];
            const std::uint32_t i1 = indices[i + 1];
            const std::uint32_t i2 = indices[i + 2];
            if (i0 >= verts.size() || i1 >= verts.size() || i2 >= verts.size()) {
                continue;
            }

            const glm::vec3 v0 = glm::vec3(worldPartMatrix * glm::vec4(verts[i0].position, 1.0f));
            const glm::vec3 v1 = glm::vec3(worldPartMatrix * glm::vec4(verts[i1].position, 1.0f));
            const glm::vec3 v2 = glm::vec3(worldPartMatrix * glm::vec4(verts[i2].position, 1.0f));

            math::RayTriangleHit triHit;
            if (math::intersectRayTriangle(ray, v0, v1, v2, triHit) && triHit.t < bestT) {
                anyHit = true;
                bestT = triHit.t;
                bestPt = triHit.point;
            }
        }
    }

    if (anyHit) {
        outNearestT = bestT;
        outHitPoint = bestPt;
    }
    return anyHit;
}

PickResult PickingSystem::pickObject(const math::Ray& ray,
                                     const scene::Scene& scene,
                                     const AssetDataLookupFn& assetLookup,
                                     const PickOptions& options)
{
    PickResult result;
    if (!ray.isValid() || !assetLookup) {
        return result;
    }

    const float wallLimitT = options.respectVisibleWalls
                                 ? computeNearestVisibleWallHit(ray, scene.getRoom())
                                 : std::numeric_limits<float>::infinity();

    float bestT = std::numeric_limits<float>::infinity();
    bool sawOccludedCandidate = false;

    for (const auto& obj : scene.getObjects()) {
        // Bỏ qua vật thể đang ẩn
        if (!obj.visible) {
            continue;
        }

        const assets::AssetData* asset = assetLookup(obj.assetId);
        if (asset == nullptr || !asset->localBounds.isValid()) {
            continue;
        }

        // Giai đoạn 1 (Broad-phase): Ray – World-space AABB
        const math::BoundingBox worldBox = obj.getWorldBounds(asset->localBounds);
        math::RayAabbHit aabbHit;
        if (!math::intersectRayAABB(ray, worldBox, aabbHit)) {
            continue;
        }

        // Nếu cả hộp bao đã xa hơn ứng viên gần nhất hiện có -> bỏ qua
        if (aabbHit.tHit >= bestT) {
            continue;
        }

        float candidateT = aabbHit.tHit;
        glm::vec3 candidatePt = aabbHit.point;
        bool refined = false;

        // Giai đoạn 2 (Narrow-phase): Ray – Triangle (Möller–Trumbore)
        if (options.useTriangleRefinement) {
            float triT = 0.0f;
            glm::vec3 triPt(0.0f);
            if (!intersectAssetTriangles(ray, obj, *asset, triT, triPt)) {
                continue;
            }
            candidateT = triT;
            candidatePt = triPt;
            refined = true;
        }

        // Kiểm tra che khuất bởi tường đang hiển thị
        if (options.respectVisibleWalls && candidateT > wallLimitT + 1e-4f) {
            sawOccludedCandidate = true;
            continue;
        }

        if (candidateT < bestT) {
            bestT = candidateT;
            result.hit = true;
            result.objectId = obj.id;
            result.distance = candidateT;
            result.hitPoint = candidatePt;
            result.usedTriangleRefinement = refined;
        }
    }

    if (!result.hit && sawOccludedCandidate) {
        result.occludedByWall = true;
    }
    return result;
}

} // namespace roomcraft::systems
