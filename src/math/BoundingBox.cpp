#include "roomcraft/math/BoundingBox.hpp"

#include <glm/common.hpp>
#include <glm/vec4.hpp>

#include <cmath>

namespace roomcraft::math {

void BoundingBox::reset()
{
    min = glm::vec3(0.0f);
    max = glm::vec3(0.0f);
    valid = false;
}

void BoundingBox::expand(const glm::vec3& point)
{
    if (!std::isfinite(point.x) || !std::isfinite(point.y) || !std::isfinite(point.z)) {
        return;
    }

    if (!valid) {
        min = point;
        max = point;
        valid = true;
        return;
    }

    min = glm::min(min, point);
    max = glm::max(max, point);
}

void BoundingBox::expand(const BoundingBox& other)
{
    if (!other.valid) {
        return;
    }
    expand(other.min);
    expand(other.max);
}

BoundingBox BoundingBox::transformed(const glm::mat4& matrix) const
{
    BoundingBox result;
    if (!valid) {
        return result;
    }

    const glm::vec3 corners[8] = {
        {min.x, min.y, min.z},
        {max.x, min.y, min.z},
        {min.x, max.y, min.z},
        {max.x, max.y, min.z},
        {min.x, min.y, max.z},
        {max.x, min.y, max.z},
        {min.x, max.y, max.z},
        {max.x, max.y, max.z}
    };

    for (const auto& corner : corners) {
        const glm::vec4 transformedCorner = matrix * glm::vec4(corner, 1.0f);
        result.expand(glm::vec3(transformedCorner));
    }

    return result;
}

glm::vec3 BoundingBox::getCenter() const
{
    if (!valid) {
        return glm::vec3(0.0f);
    }
    return 0.5f * (min + max);
}

glm::vec3 BoundingBox::getSize() const
{
    if (!valid) {
        return glm::vec3(0.0f);
    }
    return max - min;
}

glm::vec3 BoundingBox::getHalfExtents() const
{
    return 0.5f * getSize();
}

BoundingBox BoundingBox::fromVertices(const std::vector<graphics::Vertex>& vertices)
{
    BoundingBox box;
    for (const auto& v : vertices) {
        box.expand(v.position);
    }
    return box;
}

} // namespace roomcraft::math
