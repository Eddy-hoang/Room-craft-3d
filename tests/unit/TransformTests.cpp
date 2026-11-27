#include "roomcraft/scene/Transform.hpp"

#include <glm/gtc/epsilon.hpp>
#include <glm/vec4.hpp>

#include <cmath>
#include <iostream>
#include <limits>

namespace {

constexpr float kEps = 1e-4f;

bool approxEqual(float a, float b, float eps = kEps)
{
    return std::fabs(a - b) <= eps;
}

bool approxVec3(const glm::vec3& a, const glm::vec3& b, float eps = kEps)
{
    return approxEqual(a.x, b.x, eps) && approxEqual(a.y, b.y, eps) && approxEqual(a.z, b.z, eps);
}

glm::vec3 transformPoint(const glm::mat4& m, const glm::vec3& p)
{
    const glm::vec4 res = m * glm::vec4(p, 1.0f);
    return glm::vec3(res);
}

} // namespace

int runTransformTests()
{
    int failures = 0;
    const auto check = [&failures](const char* name, bool condition) {
        std::cout << "  [Transform] " << name << ": " << (condition ? "PASS" : "FAIL") << std::endl;
        if (!condition) {
            ++failures;
        }
    };

    // 1. Identity Transform
    {
        roomcraft::scene::Transform t;
        const glm::vec3 p(1.5f, -2.25f, 3.0f);
        check("Identity transform leaves point unchanged", approxVec3(transformPoint(t.getModelMatrix(), p), p));
    }

    // 2. TC-M1-06: Translation
    {
        roomcraft::scene::Transform t;
        t.position = {2.0f, -1.5f, 4.25f};
        const glm::vec3 p(1.0f, 0.5f, -1.25f);
        check("TC-M1-06 Translation", approxVec3(transformPoint(t.getModelMatrix(), p), {3.0f, -1.0f, 3.0f}));
    }

    // 3. TC-M1-07: Rotation 90 degrees around Y, X, Z (right-handed Y-up)
    {
        roomcraft::scene::Transform ry;
        ry.rotationDegrees = {0.0f, 90.0f, 0.0f};
        // Xoay +90° quanh +Y: (1, 0, 0) -> (0, 0, -1) theo quy tắc bàn tay phải
        check("TC-M1-07 Rotation +90 deg around Y",
              approxVec3(transformPoint(ry.getModelMatrix(), {1.0f, 0.0f, 0.0f}), {0.0f, 0.0f, -1.0f}));

        roomcraft::scene::Transform rx;
        rx.rotationDegrees = {90.0f, 0.0f, 0.0f};
        // Xoay +90° quanh +X: (0, 1, 0) -> (0, 0, 1)
        check("TC-M1-07 Rotation +90 deg around X",
              approxVec3(transformPoint(rx.getModelMatrix(), {0.0f, 1.0f, 0.0f}), {0.0f, 0.0f, 1.0f}));

        roomcraft::scene::Transform rz;
        rz.rotationDegrees = {0.0f, 0.0f, 90.0f};
        // Xoay +90° quanh +Z: (1, 0, 0) -> (0, 1, 0)
        check("TC-M1-07 Rotation +90 deg around Z",
              approxVec3(transformPoint(rz.getModelMatrix(), {1.0f, 0.0f, 0.0f}), {0.0f, 1.0f, 0.0f}));
    }

    // 4. TC-M1-08: Uniform & Non-uniform Scaling
    {
        roomcraft::scene::Transform tUniform;
        tUniform.scale = {2.0f, 2.0f, 2.0f};
        check("TC-M1-08 Uniform scaling 2x",
              approxVec3(transformPoint(tUniform.getModelMatrix(), {1.0f, -2.0f, 0.5f}), {2.0f, -4.0f, 1.0f}));

        roomcraft::scene::Transform tNonUniform;
        tNonUniform.scale = {2.0f, 0.5f, 3.0f};
        check("TC-M1-08 Non-uniform scaling (2, 0.5, 3)",
              approxVec3(transformPoint(tNonUniform.getModelMatrix(), {1.0f, 4.0f, -2.0f}), {2.0f, 2.0f, -6.0f}));
    }

    // 5. Combined Transformation: M = T * R * S
    //    Điểm (1, 0, 0) -> Scale (2,1,1) => (2,0,0) -> Rotate Y +90° => (0,0,-2) -> Translate (10,5,3) => (10,5,1)
    {
        roomcraft::scene::Transform t({10.0f, 5.0f, 3.0f}, {0.0f, 90.0f, 0.0f}, {2.0f, 1.0f, 1.0f});
        check("Combined M = T * R * S order verification",
              approxVec3(transformPoint(t.getModelMatrix(), {1.0f, 0.0f, 0.0f}), {10.0f, 5.0f, 1.0f}));

        t.reset();
        check("Transform::reset restores identity",
              approxVec3(t.position, {0.0f, 0.0f, 0.0f}) &&
              approxVec3(t.rotationDegrees, {0.0f, 0.0f, 0.0f}) &&
              approxVec3(t.scale, {1.0f, 1.0f, 1.0f}));
    }

    // 6. Invalid scale handling
    {
        roomcraft::scene::Transform t;
        const bool rejectZero = !t.setScaleSafe({0.0f, 1.0f, 1.0f});
        const bool rejectNegative = !t.setScaleSafe({1.0f, -2.0f, 1.0f});
        const bool rejectNan = !t.setScaleSafe({std::numeric_limits<float>::quiet_NaN(), 1.0f, 1.0f});
        check("Invalid scale values rejected (0, negative, NaN)", rejectZero && rejectNegative && rejectNan);
    }

    return failures;
}
