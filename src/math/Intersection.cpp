#include "roomcraft/math/Intersection.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace roomcraft::math {

bool intersectRayAABB(const Ray& ray, const BoundingBox& box, RayAabbHit& outHit)
{
    outHit = RayAabbHit{};
    if (!ray.isValid() || !box.isValid()) {
        return false;
    }

    if (!std::isfinite(box.min.x) || !std::isfinite(box.min.y) || !std::isfinite(box.min.z) ||
        !std::isfinite(box.max.x) || !std::isfinite(box.max.y) || !std::isfinite(box.max.z) ||
        box.min.x > box.max.x || box.min.y > box.max.y || box.min.z > box.max.z) {
        return false;
    }

    constexpr float kParallelEps = 1e-7f;
    float tMin = -std::numeric_limits<float>::infinity();
    float tMax = std::numeric_limits<float>::infinity();

    // Thuật toán Slab Method: tính khoảng [tMin, tMax] giao nhau trên lần lượt 3 trục X, Y, Z
    for (int axis = 0; axis < 3; ++axis) {
        const float originA = ray.origin[axis];
        const float dirA = ray.direction[axis];
        const float minA = box.min[axis];
        const float maxA = box.max[axis];

        if (std::fabs(dirA) < kParallelEps) {
            // Tia song song với cặp mặt phẳng trục hiện tại: nếu điểm xuất phát nằm ngoài dải -> không giao
            if (originA < minA - 1e-6f || originA > maxA + 1e-6f) {
                return false;
            }
            continue;
        }

        const float invD = 1.0f / dirA;
        float t1 = (minA - originA) * invD;
        float t2 = (maxA - originA) * invD;
        if (t1 > t2) {
            std::swap(t1, t2);
        }

        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
        if (tMin > tMax) {
            return false;
        }
    }

    // Toàn bộ hộp bao nằm phía sau gốc tia
    if (tMax < 0.0f) {
        return false;
    }

    outHit.hit = true;
    outHit.tNear = tMin;
    outHit.tFar = tMax;
    outHit.startedInside = (tMin < 0.0f);
    outHit.tHit = outHit.startedInside ? 0.0f : tMin;
    outHit.point = ray.at(outHit.tHit);
    return true;
}

bool intersectRayPlane(const Ray& ray,
                       const Plane& plane,
                       RayPlaneHit& outHit,
                       bool allowZeroT)
{
    outHit = RayPlaneHit{};
    if (!ray.isValid() || !plane.isValid()) {
        return false;
    }

    const glm::vec3 n = glm::normalize(plane.normal);
    const float denom = glm::dot(ray.direction, n);
    constexpr float kEps = 1e-6f;
    if (!std::isfinite(denom) || std::fabs(denom) < kEps) {
        return false;
    }

    const float t = glm::dot(plane.point - ray.origin, n) / denom;
    if (!std::isfinite(t)) {
        return false;
    }

    if (allowZeroT) {
        if (t < -kEps) {
            return false;
        }
    } else {
        if (t <= kEps) {
            return false;
        }
    }

    outHit.hit = true;
    outHit.t = (t < 0.0f) ? 0.0f : t;
    outHit.point = ray.at(outHit.t);
    return true;
}

bool intersectRayTriangle(const Ray& ray,
                          const glm::vec3& v0,
                          const glm::vec3& v1,
                          const glm::vec3& v2,
                          RayTriangleHit& outHit)
{
    outHit = RayTriangleHit{};
    if (!ray.isValid()) {
        return false;
    }

    // Thuật toán Möller–Trumbore (chấp nhận cả hai mặt tam giác để chọn an toàn với bề mặt mỏng như tựa ghế Bézier)
    constexpr float kEps = 1e-7f;
    const glm::vec3 edge1 = v1 - v0;
    const glm::vec3 edge2 = v2 - v0;
    const glm::vec3 pvec = glm::cross(ray.direction, edge2);
    const float det = glm::dot(edge1, pvec);

    if (!std::isfinite(det) || std::fabs(det) < kEps) {
        return false;
    }

    const float invDet = 1.0f / det;
    const glm::vec3 tvec = ray.origin - v0;
    const float u = glm::dot(tvec, pvec) * invDet;
    if (u < -1e-6f || u > 1.0f + 1e-6f) {
        return false;
    }

    const glm::vec3 qvec = glm::cross(tvec, edge1);
    const float v = glm::dot(ray.direction, qvec) * invDet;
    if (v < -1e-6f || (u + v) > 1.0f + 1e-6f) {
        return false;
    }

    const float t = glm::dot(edge2, qvec) * invDet;
    if (!std::isfinite(t) || t < 0.0f) {
        return false;
    }

    outHit.hit = true;
    outHit.t = t;
    outHit.u = u;
    outHit.v = v;
    outHit.point = ray.at(t);
    return true;
}

} // namespace roomcraft::math
