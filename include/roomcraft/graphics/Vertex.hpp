#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <type_traits>

namespace roomcraft::graphics {

// Cấu trúc đỉnh chuẩn (Interleaved Vertex Layout) cho toàn bộ Graphics Engine:
// - location 0: position (vec3, đơn vị mét trong không gian local)
// - location 1: normal   (vec3, vector pháp tuyến đơn vị dùng cho chiếu sáng về sau)
// - location 2: texCoord (vec2, tọa độ ánh xạ texture UV trong [0, 1])
struct Vertex {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec2 texCoord{0.0f, 0.0f};
};

// Đảm bảo struct đạt standard layout để dùng offsetof() hợp lệ trong C++17
static_assert(std::is_standard_layout_v<Vertex>, "Vertex must satisfy standard layout for offsetof");

} // namespace roomcraft::graphics
