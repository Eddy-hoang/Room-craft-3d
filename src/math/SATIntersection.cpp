#include "roomcraft/math/SATIntersection.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace roomcraft::math {

namespace {

// Tính bán kính hình chiếu của OBB lên trục đơn vị L:
//   r = |dot(L, u0)| * h.x + |dot(L, u1)| * h.y + |dot(L, u2)| * h.z
float projectObbRadius(const OBB& box, const glm::vec3& unitAxis)
{
    return std::fabs(glm::dot(unitAxis, box.axes[0])) * box.halfExtents.x +
           std::fabs(glm::dot(unitAxis, box.axes[1])) * box.halfExtents.y +
           std::fabs(glm::dot(unitAxis, box.axes[2])) * box.halfExtents.z;
}

} // namespace

SATResult SATIntersection::testOBB(const OBB& a,
                                   const OBB& b,
                                   float penetrationTolerance,
                                   float parallelAxisEpsilon)
{
    SATResult result;
    if (!a.isValid() || !b.isValid()) {
        result.state = ContactState::Separated;
        result.penetrationDepth = -1.0f;
        return result;
    }

    const float penEps = std::max(0.0f, penetrationTolerance);
    const float axisEpsSq = parallelAxisEpsilon * parallelAxisEpsilon;
    const glm::vec3 centerDelta = b.center - a.center;

    float minOverlap = std::numeric_limits<float>::infinity();
    glm::vec3 bestAxis(0.0f, 1.0f, 0.0f);

    auto evaluateAxis = [&](const glm::vec3& rawAxis) -> bool {
        const float lenSq = glm::dot(rawAxis, rawAxis);
        if (!std::isfinite(lenSq) || lenSq <= axisEpsSq) {
            // Trục tích có hướng suy biến do hai cạnh gần song song -> bỏ qua an toàn
            return true;
        }

        const glm::vec3 axis = rawAxis / std::sqrt(lenSq);
        ++result.testedAxes;

        const float ra = projectObbRadius(a, axis);
        const float rb = projectObbRadius(b, axis);
        const float dist = std::fabs(glm::dot(centerDelta, axis));
        const float overlap = (ra + rb) - dist;

        if (overlap < minOverlap) {
            minOverlap = overlap;
            // Định hướng trục MTV từ tâm A sang tâm B để nhất quán
            bestAxis = (glm::dot(centerDelta, axis) < 0.0f) ? -axis : axis;
        }

        // Nếu tách rời vượt quá dung sai -> kết luận ngay lập tức theo SAT
        if (overlap < -penEps) {
            return false;
        }
        return true;
    };

    // 1. Ba trục cục bộ của OBB A (A0, A1, A2)
    for (int i = 0; i < 3; ++i) {
        if (!evaluateAxis(a.axes[static_cast<std::size_t>(i)])) {
            result.state = ContactState::Separated;
            result.penetrationDepth = minOverlap;
            result.separationAxis = bestAxis;
            return result;
        }
    }

    // 2. Ba trục cục bộ của OBB B (B0, B1, B2)
    for (int j = 0; j < 3; ++j) {
        if (!evaluateAxis(b.axes[static_cast<std::size_t>(j)])) {
            result.state = ContactState::Separated;
            result.penetrationDepth = minOverlap;
            result.separationAxis = bestAxis;
            return result;
        }
    }

    // 3. Chín trục tích có hướng (A_i x B_j)
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            const glm::vec3 crossAxis = glm::cross(a.axes[static_cast<std::size_t>(i)],
                                                   b.axes[static_cast<std::size_t>(j)]);
            if (!evaluateAxis(crossAxis)) {
                result.state = ContactState::Separated;
                result.penetrationDepth = minOverlap;
                result.separationAxis = bestAxis;
                return result;
            }
        }
    }

    // Không tìm thấy trục phân tách thực sự -> phân loại Touching vs Penetrating
    result.penetrationDepth = minOverlap;
    result.separationAxis = bestAxis;
    if (minOverlap > penEps) {
        result.state = ContactState::Penetrating;
    } else if (minOverlap >= -penEps) {
        result.state = ContactState::Touching;
    } else {
        result.state = ContactState::Separated;
    }
    return result;
}

} // namespace roomcraft::math
