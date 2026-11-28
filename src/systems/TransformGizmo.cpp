#include "roomcraft/systems/TransformGizmo.hpp"

#include "roomcraft/math/Intersection.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace roomcraft::systems {

float TransformGizmo::computeHandleLength(const glm::vec3& origin, const graphics::Camera& camera)
{
    if (camera.getProjectionMode() == graphics::ProjectionMode::Orthographic) {
        return std::clamp(camera.getOrthoHalfHeight() * 0.26f, 0.30f, 3.0f);
    }
    const float dist = glm::length(camera.getPosition() - origin);
    if (!std::isfinite(dist) || dist <= 1e-3f) {
        return 0.75f;
    }
    return std::clamp(dist * 0.16f, 0.30f, 3.0f);
}

math::BoundingBox TransformGizmo::getAxisHandleBounds(const glm::vec3& origin,
                                                      float handleLength,
                                                      AxisConstraint axis)
{
    math::BoundingBox box;
    if (!std::isfinite(handleLength) || handleLength <= 1e-3f) {
        return box;
    }

    const float startOffset = handleLength * 0.12f;
    const float endOffset = handleLength * 1.12f;
    const float radius = std::max(0.045f, handleLength * 0.105f);

    switch (axis) {
        case AxisConstraint::X:
            box.expand(origin + glm::vec3(startOffset, -radius, -radius));
            box.expand(origin + glm::vec3(endOffset,    radius,  radius));
            break;
        case AxisConstraint::Y:
            box.expand(origin + glm::vec3(-radius, startOffset, -radius));
            box.expand(origin + glm::vec3( radius, endOffset,    radius));
            break;
        case AxisConstraint::Z:
            box.expand(origin + glm::vec3(-radius, -radius, startOffset));
            box.expand(origin + glm::vec3( radius,  radius, endOffset));
            break;
        default:
            break;
    }
    return box;
}

AxisConstraint TransformGizmo::pickAxis(const math::Ray& ray,
                                        const glm::vec3& origin,
                                        float handleLength,
                                        float* outHitDistance)
{
    if (!ray.isValid() || !std::isfinite(handleLength) || handleLength <= 1e-3f) {
        return AxisConstraint::None;
    }

    AxisConstraint bestAxis = AxisConstraint::None;
    float bestT = std::numeric_limits<float>::infinity();

    const AxisConstraint axes[3] = {AxisConstraint::X, AxisConstraint::Y, AxisConstraint::Z};
    for (AxisConstraint axis : axes) {
        const math::BoundingBox box = getAxisHandleBounds(origin, handleLength, axis);
        math::RayAabbHit hit;
        if (math::intersectRayAABB(ray, box, hit) && hit.tHit < bestT) {
            bestT = hit.tHit;
            bestAxis = axis;
        }
    }

    if (outHitDistance != nullptr && bestAxis != AxisConstraint::None) {
        *outHitDistance = bestT;
    }
    return bestAxis;
}

} // namespace roomcraft::systems
