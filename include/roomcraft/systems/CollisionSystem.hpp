#pragma once

#include "roomcraft/assets/AssetData.hpp"
#include "roomcraft/math/BoundingBox.hpp"
#include "roomcraft/math/OBB.hpp"
#include "roomcraft/math/SATIntersection.hpp"
#include "roomcraft/scene/ObjectId.hpp"
#include "roomcraft/scene/Room.hpp"
#include "roomcraft/scene/Scene.hpp"

#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace roomcraft::systems {

// Chế độ kiểm tra va chạm & bố trí nội thất (M8.4).
// Mặc định là WarningOnly để cảnh báo trực quan mà không cản trở sáng tạo bố trí.
enum class PlacementValidationMode {
    Off = 0,
    WarningOnly = 1,
    PreventInvalid = 2
};

inline const char* placementValidationModeToString(PlacementValidationMode mode)
{
    switch (mode) {
        case PlacementValidationMode::Off:            return "Off";
        case PlacementValidationMode::WarningOnly:    return "Warning Only";
        case PlacementValidationMode::PreventInvalid: return "Prevent Invalid";
        default:                                      return "Unknown";
    }
}

struct CollisionOptions {
    // Chính sách ẩn đối tượng: mặc định đối tượng ẩn (visible=false) KHÔNG tham gia kiểm tra va chạm.
    bool includeHiddenObjects = false;
    // Dung sai xuyên lún (mét): hai đồ vật chạm khít hoặc vật đặt trên mặt bàn (sai số <= 5mm) chỉ tính là Touching.
    float penetrationTolerance = 0.005f;
    // Dung sai biên phòng (mét): cho phép đặt sát tường/sàn trong sai số số học 5mm.
    float boundaryTolerance = 0.005f;
    // Cho phép nội thất đặt lên thảm trải sàn (carpet/rug mỏng sát sàn) mà không coi là va chạm lỗi.
    bool allowRugOverlap = true;
};

// Kết quả kiểm tra giới hạn căn phòng cho một hộp bao đã biến đổi trong không gian thế giới.
struct RoomBoundaryResult {
    bool valid = true;
    bool exceedsWest = false;    // min.x < -width/2
    bool exceedsEast = false;    // max.x > +width/2
    bool exceedsNorth = false;   // min.z < -length/2
    bool exceedsSouth = false;   // max.z > +length/2
    bool belowFloor = false;     // min.y < 0
    bool aboveCeiling = false;   // max.y > height
    float maxViolationDepth = 0.0f;

    std::string formatSummary() const;
};

// Kết quả va chạm giữa một cặp đối tượng (luôn có objectA < objectB, không trùng lặp, không tự va chạm).
struct CollisionPair {
    scene::ObjectId objectA = scene::kInvalidObjectId;
    scene::ObjectId objectB = scene::kInvalidObjectId;
    std::string nameA;
    std::string nameB;
    math::ContactState contactState = math::ContactState::Separated;
    float penetrationDepth = 0.0f;
    glm::vec3 separationAxis{0.0f, 1.0f, 0.0f};
};

// Trạng thái kiểm tra tổng hợp cho từng đối tượng riêng lẻ
struct ObjectCollisionStatus {
    scene::ObjectId objectId = scene::kInvalidObjectId;
    math::BoundingBox worldAabb;
    math::OBB worldObb;
    bool boundsValid = false;
    RoomBoundaryResult boundary;
    std::vector<scene::ObjectId> collidingObjectIds;
    std::vector<std::string> collidingObjectNames;

    bool hasCollision() const { return !collidingObjectIds.empty(); }
    bool hasBoundaryViolation() const { return !boundary.valid; }
    bool hasAnyIssue() const { return hasCollision() || hasBoundaryViolation(); }
};

// Báo cáo kiểm tra toàn cảnh (Scene-wide Collision Report)
struct SceneCollisionReport {
    std::vector<CollisionPair> collisions;
    std::vector<scene::ObjectId> outOfBoundsObjectIds;
    std::unordered_map<scene::ObjectId, ObjectCollisionStatus> objectStatuses;

    std::size_t activeObjects = 0;
    std::size_t broadPhasePairsTested = 0;
    std::size_t broadPhaseOverlaps = 0;
    std::size_t narrowPhaseSatTests = 0;
    double elapsedMs = 0.0;

    bool hasAnyIssues() const { return !collisions.empty() || !outOfBoundsObjectIds.empty(); }
    std::size_t getAffectedObjectCount() const;
    const ObjectCollisionStatus* findStatus(scene::ObjectId id) const;
};

// Bộ kiểm tra giới hạn căn phòng theo tọa độ thế giới (M8.3).
// Kiểm tra hộp bao đã biến đổi (từ OBB/AABB thế giới), KHÔNG chỉ kiểm tra gốc vị trí,
// và tôn trọng các vị trí đặt nâng hợp lệ (ví dụ đèn/máy tính trên mặt bàn y=0.75m).
class RoomBoundaryValidator {
public:
    static RoomBoundaryResult validateBounds(const math::BoundingBox& worldBounds,
                                             const scene::Room& room,
                                             float tolerance = 0.005f);
};

// Hệ thống kiểm tra va chạm 2 giai đoạn thuần CPU (M8.1 - M8.4):
//   Giai đoạn 1 (Broad-Phase) : AABB vs AABB
//   Giai đoạn 2 (Narrow-Phase): OBB vs OBB bằng Separating Axis Theorem (15 trục)
// Lưu ý: Đây là hệ thống kiểm tra va chạm hình học hộp bao cho bố trí nội thất,
// KHÔNG phải Physics Engine hay kiểm tra tam giác chính xác.
class CollisionSystem {
public:
    using LocalBoundsLookupFn = std::function<math::BoundingBox(const std::string& assetId)>;

    // Nhận diện vật phủ sàn mỏng (thảm trải sàn) để không gây cảnh báo giả khi đặt bàn/ghế/giường lên thảm
    static bool isFloorCovering(const scene::SceneObject& obj, const math::BoundingBox& worldAabb);

    // Kiểm tra cặp hai đối tượng (Broad-phase AABB -> Narrow-phase OBB SAT)
    static math::SATResult testObjectPair(const math::BoundingBox& aabbA,
                                          const math::OBB& obbA,
                                          const math::BoundingBox& aabbB,
                                          const math::OBB& obbB,
                                          float penetrationTolerance = 0.005f,
                                          bool* outPassedBroadPhase = nullptr);

    // Kiểm tra toàn bộ Scene (dùng cho đồng bộ sau thay đổi phòng/load project/benchmark)
    static SceneCollisionReport validateScene(const scene::Scene& scene,
                                              const LocalBoundsLookupFn& localBoundsLookup,
                                              const CollisionOptions& options = {});

    // Kiểm tra tăng dần O(N) cho một đối tượng với Transform ứng viên (dùng khi đang kéo/xoay/scale/đặt đồ)
    static ObjectCollisionStatus validateCandidateObject(const scene::Scene& scene,
                                                         scene::ObjectId objectId,
                                                         const std::string& assetId,
                                                         const scene::Transform& candidateTransform,
                                                         const LocalBoundsLookupFn& localBoundsLookup,
                                                         const CollisionOptions& options = {});
};

} // namespace roomcraft::systems
