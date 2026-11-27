#pragma once

#include "roomcraft/math/BoundingBox.hpp"
#include "roomcraft/math/Plane.hpp"
#include "roomcraft/math/Ray.hpp"

#include <glm/vec3.hpp>

namespace roomcraft::math {

struct RayAabbHit {
    bool hit = false;
    float tNear = 0.0f;
    float tFar = 0.0f;
    float tHit = 0.0f;      // Khoảng cách dương gần nhất dọc theo tia (0 nếu điểm xuất phát nằm trong box)
    bool startedInside = false;
    glm::vec3 point{0.0f};
};

struct RayPlaneHit {
    bool hit = false;
    float t = 0.0f;
    glm::vec3 point{0.0f};
};

struct RayTriangleHit {
    bool hit = false;
    float t = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    glm::vec3 point{0.0f};
};

// Kiểm tra giao giữa Tia và Hộp bao căn trục AABB bằng thuật toán Slab Method (xử lý an toàn tia song song,
// hướng âm, điểm xuất phát nằm bên trong box và hộp bao suy biến / không hợp lệ).
bool intersectRayAABB(const Ray& ray, const BoundingBox& box, RayAabbHit& outHit);

// Kiểm tra giao giữa Tia và Mặt phẳng: t = dot(P0 - O, N) / dot(D, N)
bool intersectRayPlane(const Ray& ray,
                       const Plane& plane,
                       RayPlaneHit& outHit,
                       bool allowZeroT = true);

// Kiểm tra giao giữa Tia và Tam giác bằng thuật toán Möller–Trumbore (Narrow-phase Picking)
bool intersectRayTriangle(const Ray& ray,
                          const glm::vec3& v0,
                          const glm::vec3& v1,
                          const glm::vec3& v2,
                          RayTriangleHit& outHit);

} // namespace roomcraft::math
