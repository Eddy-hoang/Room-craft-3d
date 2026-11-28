#pragma once

#include "roomcraft/graphics/Camera.hpp"
#include "roomcraft/math/BoundingBox.hpp"
#include "roomcraft/math/Ray.hpp"
#include "roomcraft/systems/InteractionState.hpp"

#include <glm/vec3.hpp>

namespace roomcraft::systems {

// Move Gizmo 3 trục (X Đỏ, Y Xanh lá, Z Xanh dương) thuần CPU cho hit-testing & kích thước ổn định trên màn hình.
// Tái sử dụng trực tiếp TransformController + InteractionState + AxisConstraint (không thêm dependency ngoài).
class TransformGizmo {
public:
    // Tính chiều dài tay cầm gizmo theo khoảng cách camera để giữ kích thước hiển thị tương đối ổn định
    static float computeHandleLength(const glm::vec3& origin, const graphics::Camera& camera);

    // Hộp bao va chạm thật (hit-test box) cho từng tay cầm trục X / Y / Z
    static math::BoundingBox getAxisHandleBounds(const glm::vec3& origin,
                                                 float handleLength,
                                                 AxisConstraint axis);

    // Kiểm tra giao tia chuột với 3 tay cầm trục; trả về trục gần nhất bị trúng hoặc AxisConstraint::None
    static AxisConstraint pickAxis(const math::Ray& ray,
                                   const glm::vec3& origin,
                                   float handleLength,
                                   float* outHitDistance = nullptr);
};

} // namespace roomcraft::systems
