#pragma once

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include <cmath>

namespace roomcraft::math {

// Mặt phẳng trong không gian 3D xác định bởi một điểm thuộc mặt phẳng P0 và vector pháp tuyến đơn vị N:
//   dot(P - point, normal) = 0
struct Plane {
    glm::vec3 point{0.0f, 0.0f, 0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};

    bool isValid() const
    {
        const bool finiteP = std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z);
        const bool finiteN = std::isfinite(normal.x) && std::isfinite(normal.y) && std::isfinite(normal.z);
        if (!finiteP || !finiteN) {
            return false;
        }
        return glm::length(normal) > 1e-6f;
    }

    // Tạo mặt phẳng nằm ngang tại độ cao Y (dùng cho thao tác kéo thả vật thể giữ nguyên cao độ Y)
    static Plane horizontalAtY(float y)
    {
        return Plane{glm::vec3(0.0f, y, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)};
    }
};

} // namespace roomcraft::math
