#pragma once

#include "roomcraft/graphics/Vertex.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <vector>

namespace roomcraft::math {

// Hộp bao căn trục trong không gian cục bộ (Local-space Axis-Aligned Bounding Box - AABB).
// Trong Milestone M2, BoundingBox phục vụ siêu dữ liệu hình học, kiểm tra kích thước
// và hợp nhất kích thước của các món nội thất phân cấp (chưa phải CollisionSystem của M8).
struct BoundingBox {
    glm::vec3 min{0.0f, 0.0f, 0.0f};
    glm::vec3 max{0.0f, 0.0f, 0.0f};
    bool valid = false;

    void reset();
    void expand(const glm::vec3& point);
    void expand(const BoundingBox& other);

    // Biến đổi 8 góc của hộp bao bằng ma trận 4x4 và trả về AABB mới bao bọc chúng
    BoundingBox transformed(const glm::mat4& matrix) const;

    bool isValid() const { return valid; }
    glm::vec3 getCenter() const;
    glm::vec3 getSize() const;
    glm::vec3 getHalfExtents() const;

    // Tính AABB trực tiếp từ danh sách đỉnh
    static BoundingBox fromVertices(const std::vector<graphics::Vertex>& vertices);
};

} // namespace roomcraft::math
