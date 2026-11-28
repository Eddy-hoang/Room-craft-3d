#include "roomcraft/math/OBB.hpp"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace roomcraft::math {

namespace {

bool isFiniteVec3(const glm::vec3& v)
{
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

glm::mat3 buildRotationMatrix(const glm::vec3& rotationDegrees)
{
    glm::mat4 r(1.0f);
    r = glm::rotate(r, glm::radians(rotationDegrees.y), glm::vec3(0.0f, 1.0f, 0.0f));
    r = glm::rotate(r, glm::radians(rotationDegrees.x), glm::vec3(1.0f, 0.0f, 0.0f));
    r = glm::rotate(r, glm::radians(rotationDegrees.z), glm::vec3(0.0f, 0.0f, 1.0f));
    return glm::mat3(r);
}

} // namespace

bool OBB::isValid() const
{
    if (!valid || !isFiniteVec3(center) || !isFiniteVec3(halfExtents)) {
        return false;
    }
    if (halfExtents.x < 0.0f || halfExtents.y < 0.0f || halfExtents.z < 0.0f) {
        return false;
    }
    for (const auto& axis : axes) {
        if (!isFiniteVec3(axis)) {
            return false;
        }
        const float lenSq = glm::dot(axis, axis);
        if (std::fabs(lenSq - 1.0f) > 1e-2f) {
            return false;
        }
    }
    return true;
}

OBB OBB::fromLocalBoundsAndTransform(const BoundingBox& localBounds,
                                     const scene::Transform& transform,
                                     std::string* outError)
{
    OBB obb;
    if (!localBounds.isValid()) {
        if (outError != nullptr) *outError = "Local bounding box is invalid.";
        return obb;
    }
    if (!isFiniteVec3(localBounds.min) || !isFiniteVec3(localBounds.max) ||
        localBounds.min.x > localBounds.max.x ||
        localBounds.min.y > localBounds.max.y ||
        localBounds.min.z > localBounds.max.z) {
        if (outError != nullptr) *outError = "Local bounding box bounds are malformed.";
        return obb;
    }
    if (!isFiniteVec3(transform.position) || !isFiniteVec3(transform.rotationDegrees)) {
        if (outError != nullptr) *outError = "Transform position/rotation must be finite.";
        return obb;
    }
    if (!scene::Transform::isValidScale(transform.scale)) {
        if (outError != nullptr) *outError = "Transform scale must be finite and strictly positive.";
        return obb;
    }

    const glm::vec3 localCenter = localBounds.getCenter();
    const glm::vec3 localHalf = localBounds.getHalfExtents();
    const glm::mat3 rot = buildRotationMatrix(transform.rotationDegrees);

    // Chuẩn hóa nhẹ 3 trục cơ sở để tránh trôi sai số dấu phẩy động sau nhiều phép xoay
    obb.axes[0] = glm::normalize(rot[0]);
    obb.axes[1] = glm::normalize(rot[1]);
    obb.axes[2] = glm::normalize(rot[2]);

    const glm::vec3 scaledLocalCenter = localCenter * transform.scale;
    obb.center = transform.position + rot * scaledLocalCenter;
    obb.halfExtents = glm::vec3(
        std::fabs(transform.scale.x) * localHalf.x,
        std::fabs(transform.scale.y) * localHalf.y,
        std::fabs(transform.scale.z) * localHalf.z);
    obb.valid = isFiniteVec3(obb.center) && isFiniteVec3(obb.halfExtents);
    return obb;
}

BoundingBox OBB::toWorldAABB() const
{
    BoundingBox aabb;
    if (!isValid()) {
        return aabb;
    }

    // Chiếu bán kích thước của OBB lên 3 trục thế giới X, Y, Z:
    //   e_world[k] = |axes[0][k]| * h.x + |axes[1][k]| * h.y + |axes[2][k]| * h.z
    const glm::vec3 worldHalf(
        std::fabs(axes[0].x) * halfExtents.x + std::fabs(axes[1].x) * halfExtents.y + std::fabs(axes[2].x) * halfExtents.z,
        std::fabs(axes[0].y) * halfExtents.x + std::fabs(axes[1].y) * halfExtents.y + std::fabs(axes[2].y) * halfExtents.z,
        std::fabs(axes[0].z) * halfExtents.x + std::fabs(axes[1].z) * halfExtents.y + std::fabs(axes[2].z) * halfExtents.z
    );

    aabb.min = center - worldHalf;
    aabb.max = center + worldHalf;
    aabb.valid = true;
    return aabb;
}

std::array<glm::vec3, 8> OBB::getCorners() const
{
    const glm::vec3 ex = axes[0] * halfExtents.x;
    const glm::vec3 ey = axes[1] * halfExtents.y;
    const glm::vec3 ez = axes[2] * halfExtents.z;

    return {
        center - ex - ey - ez,
        center + ex - ey - ez,
        center - ex + ey - ez,
        center + ex + ey - ez,
        center - ex - ey + ez,
        center + ex - ey + ez,
        center - ex + ey + ez,
        center + ex + ey + ez
    };
}

AabbIntersectionResult intersectAABB(const BoundingBox& a,
                                     const BoundingBox& b,
                                     float penetrationTolerance)
{
    AabbIntersectionResult res;
    if (!a.isValid() || !b.isValid()) {
        res.state = ContactState::Separated;
        res.minOverlap = -1.0f;
        return res;
    }

    const float eps = std::max(0.0f, penetrationTolerance);
    const float ox = std::min(a.max.x, b.max.x) - std::max(a.min.x, b.min.x);
    const float oy = std::min(a.max.y, b.max.y) - std::max(a.min.y, b.min.y);
    const float oz = std::min(a.max.z, b.max.z) - std::max(a.min.z, b.min.z);

    res.minOverlap = std::min({ox, oy, oz});
    if (res.minOverlap < -eps) {
        res.state = ContactState::Separated;
    } else if (res.minOverlap > eps) {
        res.state = ContactState::Penetrating;
    } else {
        res.state = ContactState::Touching;
    }
    return res;
}

} // namespace roomcraft::math
