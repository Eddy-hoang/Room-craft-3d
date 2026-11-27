#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace roomcraft::scene {

// Lớp Transform biểu diễn phép biến đổi không gian của vật thể 3D trong hệ tọa độ tay phải, Y-up, đơn vị mét.
//
// Quy ước toán học (vector cột - column vectors):
//   Model Matrix: M = T * R * S
//   - T: Ma trận tịnh tiến theo position (mét)
//   - R: Ma trận xoay Euler lưu theo ĐỘ (degrees), kết hợp theo thứ tự Y -> X -> Z:
//        R = R_y(yaw) * R_x(pitch) * R_z(roll)
//        (tức điểm local chịu Scale -> Roll Z -> Pitch X -> Yaw Y -> Translate)
//   - S: Ma trận tỷ lệ theo từng trục (scale.x, scale.y, scale.z)
class Transform {
public:
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 rotationDegrees{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};

    Transform() = default;
    Transform(const glm::vec3& pos,
              const glm::vec3& rotDeg = glm::vec3(0.0f),
              const glm::vec3& scl = glm::vec3(1.0f));

    // Tính ma trận Model 4x4 theo công thức M = T * R * S
    glm::mat4 getModelMatrix() const;

    // Đặt lại về phép biến đổi đơn vị (Identity)
    void reset();

    // Kiểm tra và thiết lập scale hợp lệ (hữu hạn và lớn hơn ngưỡng dương tối thiểu)
    static bool isValidScale(const glm::vec3& candidateScale, float minScale = 1e-4f);
    bool setScaleSafe(const glm::vec3& candidateScale, float minScale = 1e-4f);
};

} // namespace roomcraft::scene
