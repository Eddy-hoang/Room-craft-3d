#pragma once

#include "roomcraft/math/BoundingBox.hpp"
#include "roomcraft/scene/Transform.hpp"

#include <glm/vec3.hpp>

#include <array>
#include <string>

namespace roomcraft::math {

// Phân loại trạng thái tiếp xúc hình học giữa hai hộp bao (AABB hoặc OBB).
// - Separated   : tách rời hoàn toàn (tồn tại trục phân tách với khoảng hở dương)
// - Touching    : tiếp xúc bề mặt (ví dụ: đèn đặt trên mặt bàn y=0.75m, tủ kê sát nhau)
// - Penetrating : giao nhau thực sự vượt quá ngưỡng dung sai xuyên lún (penetrationTolerance)
enum class ContactState {
    Separated = 0,
    Touching = 1,
    Penetrating = 2
};

inline const char* contactStateToString(ContactState state)
{
    switch (state) {
        case ContactState::Separated:   return "Separated";
        case ContactState::Touching:    return "Touching";
        case ContactState::Penetrating: return "Penetrating";
        default:                        return "Unknown";
    }
}

struct AabbIntersectionResult {
    ContactState state = ContactState::Separated;
    float minOverlap = -1.0f; // > 0 khi xuyên lún, ~0 khi tiếp xúc, < 0 khi tách rời

    bool isSeparated() const { return state == ContactState::Separated; }
    bool isTouching() const { return state == ContactState::Touching; }
    bool isPenetrating() const { return state == ContactState::Penetrating; }
    bool isTouchingOrPenetrating() const { return state != ContactState::Separated; }
};

// Hộp bao có hướng trong không gian thế giới (World-space Oriented Bounding Box - OBB).
//
// Quy ước toán học & giới hạn (M8.1):
// - RoomCraft 3D dùng biến đổi TRS không có biến dạng cắt (no shear):
//       M = T * R_y(yaw) * R_x(pitch) * R_z(roll) * S
//   với scale dương hữu hạn (scale.x, scale.y, scale.z >= 1e-4).
// - Do S tác động TRƯỚC ma trận xoay trực chuẩn R, hộp cục bộ AABB sau khi co giãn vẫn trực giao
//   và chỉ xoay theo các trục cơ sở trực chuẩn của R:
//       axes[0] = R * (1, 0, 0), axes[1] = R * (0, 1, 0), axes[2] = R * (0, 0, 1)
//       center  = T + R * (S * localCenter)
//       halfExtents = (|S_x| * e_x, |S_y| * e_y, |S_z| * e_z)
// - Lưu ý: một ma trận affine tùy ý có thành phần shear KHÔNG thể biểu diễn chính xác bằng OBB;
//   hàm dựng tường minh kiểm tra tính hợp lệ của Transform (hữu hạn, scale dương, không suy biến).
struct OBB {
    glm::vec3 center{0.0f, 0.0f, 0.0f};
    std::array<glm::vec3, 3> axes = {
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    };
    glm::vec3 halfExtents{0.0f, 0.0f, 0.0f};
    bool valid = false;

    bool isValid() const;

    // Dựng OBB chính xác trong không gian thế giới từ Local AABB và Transform TRS hợp lệ.
    // Từ chối (trả về valid=false) khi localBounds không hợp lệ, vị trí/góc quay không hữu hạn,
    // hoặc scale <= 0 / không hữu hạn / suy biến.
    static OBB fromLocalBoundsAndTransform(const BoundingBox& localBounds,
                                           const scene::Transform& transform,
                                           std::string* outError = nullptr);

    // Chuyển OBB sang World-space AABB nhỏ nhất bao bọc nó (dùng cho broad-phase & biên phòng).
    // Công thức: worldHalfExtent_i = sum_j |axes[j][i]| * halfExtents[j]
    BoundingBox toWorldAABB() const;

    // Trả về 8 đỉnh góc trong không gian thế giới
    std::array<glm::vec3, 8> getCorners() const;
};

// Kiểm tra giao nhau hai hộp AABB trong không gian thế giới, phân biệt tách rời / tiếp xúc / xuyên lún.
// - penetrationTolerance: ngưỡng xuyên lún tối thiểu (mét) để coi là Penetrating (mặc định 1e-4 m).
AabbIntersectionResult intersectAABB(const BoundingBox& a,
                                     const BoundingBox& b,
                                     float penetrationTolerance = 1e-4f);

} // namespace roomcraft::math
