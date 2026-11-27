#include "roomcraft/geometry/GeometryUtils.hpp"

#include <cmath>

namespace roomcraft::geometry {

bool GeometryUtils::isPositiveFinite(float value, float minVal, float maxVal)
{
    return std::isfinite(value) && value >= minVal && value <= maxVal;
}

bool GeometryUtils::isNonNegativeFinite(float value, float maxVal)
{
    return std::isfinite(value) && value >= 0.0f && value <= maxVal;
}

bool GeometryUtils::isFiniteVec3(const glm::vec3& v)
{
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

MeshData GeometryUtils::buildBoundingBoxLines(const math::BoundingBox& box)
{
    MeshData lines;
    lines.topology = graphics::PrimitiveTopology::Lines;

    if (!box.isValid()) {
        return lines;
    }

    const glm::vec3& mn = box.min;
    const glm::vec3& mx = box.max;
    const glm::vec3 up(0.0f, 1.0f, 0.0f);

    lines.vertices = {
        {{mn.x, mn.y, mn.z}, up, {0.0f, 0.0f}}, // 0
        {{mx.x, mn.y, mn.z}, up, {0.0f, 0.0f}}, // 1
        {{mx.x, mn.y, mx.z}, up, {0.0f, 0.0f}}, // 2
        {{mn.x, mn.y, mx.z}, up, {0.0f, 0.0f}}, // 3
        {{mn.x, mx.y, mn.z}, up, {0.0f, 0.0f}}, // 4
        {{mx.x, mx.y, mn.z}, up, {0.0f, 0.0f}}, // 5
        {{mx.x, mx.y, mx.z}, up, {0.0f, 0.0f}}, // 6
        {{mn.x, mx.y, mx.z}, up, {0.0f, 0.0f}}  // 7
    };

    lines.indices = {
        0, 1, 1, 2, 2, 3, 3, 0, // Mặt đáy
        4, 5, 5, 6, 6, 7, 7, 4, // Mặt đỉnh
        0, 4, 1, 5, 2, 6, 3, 7  // 4 cạnh đứng
    };

    lines.recomputeBoundingBox();
    return lines;
}

MeshData GeometryUtils::buildVertexNormalLines(const MeshData& surfaceMesh, float normalLength)
{
    MeshData lines;
    lines.topology = graphics::PrimitiveTopology::Lines;

    if (surfaceMesh.vertices.empty() || !isPositiveFinite(normalLength, 1e-4f, 10.0f)) {
        return lines;
    }

    lines.vertices.reserve(surfaceMesh.vertices.size() * 2);
    lines.indices.reserve(surfaceMesh.vertices.size() * 2);

    const glm::vec3 up(0.0f, 1.0f, 0.0f);
    for (const auto& v : surfaceMesh.vertices) {
        const auto baseIdx = static_cast<std::uint32_t>(lines.vertices.size());
        lines.vertices.push_back({v.position, up, {0.0f, 0.0f}});
        lines.vertices.push_back({v.position + v.normal * normalLength, up, {1.0f, 0.0f}});
        lines.indices.push_back(baseIdx);
        lines.indices.push_back(baseIdx + 1);
    }

    lines.recomputeBoundingBox();
    return lines;
}

} // namespace roomcraft::geometry
