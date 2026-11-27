#pragma once

#include "roomcraft/geometry/MeshData.hpp"
#include "roomcraft/math/BoundingBox.hpp"

#include <glm/vec3.hpp>

#include <string>

namespace roomcraft::geometry {

// Ngưỡng giới hạn tham số an toàn cho toàn bộ Geometry Engine (tránh cấp phát bộ nhớ quá mức)
namespace limits {
constexpr float kMinDimension = 1e-4f;
constexpr float kMaxDimension = 100.0f;
constexpr int kMinRadialSegments = 3;
constexpr int kMaxRadialSegments = 256;
constexpr int kMinLatitudeSegments = 3;
constexpr int kMaxLatitudeSegments = 128;
constexpr int kMinSurfaceSegments = 1;
constexpr int kMaxSurfaceSegments = 128;
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 6.28318530717958647692f;
} // namespace limits

// Các hàm tiện ích hỗ trợ kiểm tra tham số và tạo dữ liệu trực quan hóa hình học (thuần CPU)
class GeometryUtils {
public:
    static bool isPositiveFinite(float value, float minVal = limits::kMinDimension, float maxVal = limits::kMaxDimension);
    static bool isNonNegativeFinite(float value, float maxVal = limits::kMaxDimension);
    static bool isFiniteVec3(const glm::vec3& v);

    // Sinh lưới đường thẳng (GL_LINES) biểu diễn 12 cạnh của BoundingBox
    static MeshData buildBoundingBoxLines(const math::BoundingBox& box);

    // Sinh các đoạn thẳng ngắn (GL_LINES) biểu diễn vector pháp tuyến tại mỗi đỉnh của surface mesh
    static MeshData buildVertexNormalLines(const MeshData& surfaceMesh, float normalLength = 0.08f);
};

} // namespace roomcraft::geometry
