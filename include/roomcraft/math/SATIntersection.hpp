#pragma once

#include "roomcraft/math/OBB.hpp"

#include <glm/vec3.hpp>

namespace roomcraft::math {

struct SATResult {
    ContactState state = ContactState::Separated;
    float penetrationDepth = -1.0f; // Độ xuyên lún theo trục MTV (mét): > 0 xuyên lún, ~0 tiếp xúc, < 0 tách rời
    glm::vec3 separationAxis{0.0f, 1.0f, 0.0f}; // Vector đơn vị trục phân tách (hướng từ A sang B)
    int testedAxes = 0;

    bool isSeparated() const { return state == ContactState::Separated; }
    bool isTouching() const { return state == ContactState::Touching; }
    bool isPenetrating() const { return state == ContactState::Penetrating; }
};

// Kiểm tra giao nhau hai hộp OBB trong không gian 3D theo Định lý Trục Phân tách
// (Separating Axis Theorem - SAT).
//
// Nguyên lý toán học:
// Hai hình lồi 3D không giao nhau khi và chỉ khi tồn tại một trục L mà hình chiếu của chúng
// lên L không chồng lấn. Đối với hai OBB A và B, tối đa cần xét 15 trục ứng viên:
//   - 3 trục cục bộ của A: A0, A1, A2
//   - 3 trục cục bộ của B: B0, B1, B2
//   - 9 trục tích có hướng: A_i x B_j (i, j in {0, 1, 2})
//
// Xử lý ổn định số học:
// - Khi A_i gần song song với B_j (||A_i x B_j|| <= parallelAxisEpsilon), trục tích có hướng
//   bị suy biến nên bỏ qua một cách an toàn (khi đó mặt phẳng phân tách đã được phủ bởi các trục
//   mặt A_i / B_j).
// - Mọi trục hợp lệ đều được chuẩn hóa đơn vị để penetrationDepth đo bằng đơn vị mét thực,
//   đảm bảo kết quả đối xứng testOBB(A, B) == testOBB(B, A) và nhất quán ngưỡng Touching/Penetrating.
class SATIntersection {
public:
    static SATResult testOBB(const OBB& a,
                             const OBB& b,
                             float penetrationTolerance = 1e-4f,
                             float parallelAxisEpsilon = 1e-5f);
};

} // namespace roomcraft::math
