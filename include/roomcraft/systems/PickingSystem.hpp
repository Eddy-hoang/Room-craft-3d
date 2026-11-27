#pragma once

#include "roomcraft/assets/AssetData.hpp"
#include "roomcraft/math/Ray.hpp"
#include "roomcraft/scene/ObjectId.hpp"
#include "roomcraft/scene/Scene.hpp"

#include <glm/vec3.hpp>

#include <functional>
#include <string>

namespace roomcraft::systems {

struct PickOptions {
    bool useTriangleRefinement = true;  // Bật kiểm tra hẹp Ray–Triangle (Möller–Trumbore) sau Ray–AABB
    bool respectVisibleWalls = true;    // Tường đang hiển thị che khuất nội thất phía sau; tường ẩn không chặn tia
};

struct PickResult {
    bool hit = false;
    scene::ObjectId objectId = scene::kInvalidObjectId;
    float distance = 0.0f;
    glm::vec3 hitPoint{0.0f};
    bool usedTriangleRefinement = false;
    bool occludedByWall = false;
};

// Hệ thống chọn vật thể bằng Ray Casting thuần CPU (RoomCraft 3D M5).
//
// Quy trình 2 giai đoạn:
// 1. Broad-phase: Kiểm tra giao Tia với World-space AABB (Slab Method) và tính khoảng cách tường che gần nhất.
// 2. Narrow-phase (mặc định bật): Kiểm tra giao Tia với từng tam giác thực của sub-mesh (Möller–Trumbore)
//    với ma trận M_world_part = M_object * M_asset_part để xuyên qua khoảng trống chân bàn / khung ghế.
//
// Chính sách đối tượng bị khóa (locked):
// - Vẫn cho phép chọn để hiển thị thông tin và mở khóa trong Properties Inspector.
// - TransformController từ chối di chuyển/xoay/scale/xóa khi locked == true.
class PickingSystem {
public:
    using AssetDataLookupFn = std::function<const assets::AssetData*(const std::string& assetId)>;

    static PickResult pickObject(const math::Ray& ray,
                                 const scene::Scene& scene,
                                 const AssetDataLookupFn& assetLookup,
                                 const PickOptions& options = {});

private:
    static float computeNearestVisibleWallHit(const math::Ray& ray, const scene::Room& room);
    static bool intersectAssetTriangles(const math::Ray& ray,
                                        const scene::SceneObject& object,
                                        const assets::AssetData& asset,
                                        float& outNearestT,
                                        glm::vec3& outHitPoint);
};

} // namespace roomcraft::systems
