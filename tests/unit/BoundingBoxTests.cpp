#include "roomcraft/math/BoundingBox.hpp"
#include "roomcraft/scene/Transform.hpp"

#include <glm/geometric.hpp>

#include <cmath>
#include <iostream>

namespace {

constexpr float kEps = 1e-4f;

bool approxVec3(const glm::vec3& a, const glm::vec3& b, float eps = kEps)
{
    return glm::length(a - b) <= eps;
}

} // namespace

int runBoundingBoxTests()
{
    int failures = 0;
    const auto check = [&failures](const char* name, bool condition) {
        std::cout << "  [BoundingBox] " << name << ": " << (condition ? "PASS" : "FAIL") << std::endl;
        if (!condition) {
            ++failures;
        }
    };

    // TC-M2-16: BoundingBox min, max, center, size & merge
    {
        roomcraft::math::BoundingBox box;
        box.expand({-1.0f, 0.0f, -0.5f});
        box.expand({ 2.0f, 3.0f,  1.5f});
        check("TC-M2-16 Expand computes accurate min, max, center, size",
              box.isValid() &&
              approxVec3(box.min, {-1.0f, 0.0f, -0.5f}) &&
              approxVec3(box.max, { 2.0f, 3.0f,  1.5f}) &&
              approxVec3(box.getCenter(), {0.5f, 1.5f, 0.5f}) &&
              approxVec3(box.getSize(), {3.0f, 3.0f, 2.0f}));

        // Biến đổi AABB bằng tịnh tiến + xoay 90° quanh trục Y (đổi trục X <-> Z)
        roomcraft::scene::Transform t({5.0f, 1.0f, 2.0f}, {0.0f, 90.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
        const auto worldBox = box.transformed(t.getModelMatrix());
        const glm::vec3 rotSize = worldBox.getSize();
        check("TC-M2-16 Transformed AABB swaps X/Z extents under 90 deg Y rotation",
              worldBox.isValid() && approxVec3(rotSize, {2.0f, 3.0f, 3.0f}));
    }

    return failures;
}
