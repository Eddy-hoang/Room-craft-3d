#pragma once

#include "roomcraft/geometry/BezierCurve.hpp"
#include "roomcraft/geometry/MeshData.hpp"

#include <glm/vec2.hpp>

#include <string>
#include <vector>

namespace roomcraft::geometry {

// Sinh bề mặt tròn xoay (Surface of Revolution) bằng cách quay đường cong biên 2D (radius, height)
// quanh trục +Y:
//   x(t, theta) = r(t) * cos(theta)
//   y(t, theta) = y(t)
//   z(t, theta) = r(t) * sin(theta)
//
// Lưu ý giới hạn M2:
// - Tạo vỏ bề mặt đơn (exterior shell) kèm tùy chọn nắp phẳng đáy/đỉnh;
//   chưa tạo thành trong/độ dày rỗng cho bình hoa.
class RevolutionSurface {
public:
    // Sinh mặt tròn xoay từ danh sách điểm biên 2D: mỗi điểm glm::vec2(radius >= 0, height)
    static bool generateFromProfile(const std::vector<glm::vec2>& profilePoints,
                                    int radialSegments,
                                    bool capBottom,
                                    bool capTop,
                                    MeshData& outMesh,
                                    std::string* outError = nullptr);

    // Sinh mặt tròn xoay từ một Cubic BezierCurve (dùng tọa độ X làm bán kính r(t) và Y làm chiều cao y(t))
    static bool generateFromBezierProfile(const BezierCurve& profileCurve,
                                          int profileSegments,
                                          int radialSegments,
                                          bool capBottom,
                                          bool capTop,
                                          MeshData& outMesh,
                                          std::string* outError = nullptr);
};

} // namespace roomcraft::geometry
