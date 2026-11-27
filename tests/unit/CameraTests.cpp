#include "roomcraft/graphics/Camera.hpp"

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

bool isFiniteMat4(const glm::mat4& m)
{
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            if (!std::isfinite(m[c][r])) {
                return false;
            }
        }
    }
    return true;
}

} // namespace

int runCameraTests()
{
    int failures = 0;
    const auto check = [&failures](const char* name, bool condition) {
        std::cout << "  [Camera] " << name << ": " << (condition ? "PASS" : "FAIL") << std::endl;
        if (!condition) {
            ++failures;
        }
    };

    // 1. TC-M1-09: Perspective Projection (điểm gần có tọa độ chiếu lớn hơn điểm xa)
    {
        roomcraft::graphics::Camera cam;
        cam.setProjectionMode(roomcraft::graphics::ProjectionMode::Perspective);
        cam.setAspectRatio(16.0f / 9.0f);
        cam.setFieldOfView(60.0f);
        cam.setClipPlanes(0.1f, 100.0f);
        cam.lookAt({0.0f, 0.0f, 5.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

        const glm::mat4 vp = cam.getProjectionMatrix() * cam.getViewMatrix();
        const glm::vec4 clipNear = vp * glm::vec4(1.0f, 0.0f, 2.0f, 1.0f); // cách camera 3m
        const glm::vec4 clipFar = vp * glm::vec4(1.0f, 0.0f, -5.0f, 1.0f); // cách camera 10m
        const float ndcNearX = clipNear.x / clipNear.w;
        const float ndcFarX = clipFar.x / clipFar.w;

        check("TC-M1-09 Perspective matrix finite and foreshortening holds",
              isFiniteMat4(vp) && (clipNear.w > 0.0f) && (clipFar.w > clipNear.w) && (ndcNearX > ndcFarX));
    }

    // 2. TC-M1-10: Orthographic Projection (vật gần và vật xa có cùng kích thước chiếu, w = 1)
    {
        roomcraft::graphics::Camera cam;
        cam.setProjectionMode(roomcraft::graphics::ProjectionMode::Orthographic);
        cam.setAspectRatio(2.0f);
        cam.setOrthoHalfHeight(4.0f); // halfWidth = 8.0 -> x=4.0 map đúng về NDC x=0.5
        cam.setClipPlanes(0.1f, 50.0f);
        cam.lookAt({0.0f, 0.0f, 10.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

        const glm::mat4 vp = cam.getProjectionMatrix() * cam.getViewMatrix();
        const glm::vec4 clipNear = vp * glm::vec4(4.0f, 2.0f, 5.0f, 1.0f);
        const glm::vec4 clipFar = vp * glm::vec4(4.0f, 2.0f, -10.0f, 1.0f);

        check("TC-M1-10 Orthographic projection preserves parallel scale",
              isFiniteMat4(vp) &&
              approxEqual(clipNear.w, 1.0f) &&
              approxEqual(clipFar.w, 1.0f) &&
              approxEqual(clipNear.x, 0.5f) &&
              approxEqual(clipFar.x, 0.5f) &&
              approxEqual(clipNear.y, 0.5f) &&
              approxEqual(clipFar.y, 0.5f));
    }

    // 3. Invalid input handling
    {
        roomcraft::graphics::Camera cam;
        const bool rejectZeroAspect = !cam.setAspectRatio(0.0f);
        const bool rejectNegAspect = !cam.setAspectRatio(-1.5f);
        const bool rejectNanAspect = !cam.setAspectRatio(std::numeric_limits<float>::quiet_NaN());
        check("Invalid aspect ratio rejected (0, negative, NaN)",
              rejectZeroAspect && rejectNegAspect && rejectNanAspect);

        const bool rejectZeroNear = !cam.setClipPlanes(0.0f, 100.0f);
        const bool rejectNegNear = !cam.setClipPlanes(-1.0f, 100.0f);
        const bool rejectFarLeNear = !cam.setClipPlanes(10.0f, 10.0f) && !cam.setClipPlanes(10.0f, 5.0f);
        check("Invalid clip planes rejected (near <= 0 or far <= near)",
              rejectZeroNear && rejectNegNear && rejectFarLeNear);

        const bool rejectSamePosTarget = !cam.lookAt({1.0f, 2.0f, 3.0f}, {1.0f, 2.0f, 3.0f});
        const bool rejectCollinearUp = !cam.lookAt({0.0f, 5.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
        const bool rejectZeroUp = !cam.lookAt({0.0f, 0.0f, 5.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
        check("Degenerate lookAt inputs rejected (pos == target, collinear/zero up)",
              rejectSamePosTarget && rejectCollinearUp && rejectZeroUp);

        const bool rejectFov = !cam.setFieldOfView(0.0f) && !cam.setFieldOfView(180.0f);
        check("Out-of-range FOV rejected", rejectFov);
    }

    // 4. Presets produce finite matrices
    {
        roomcraft::graphics::Camera cam;
        cam.setTopOrthoPreset();
        const bool topOk = isFiniteMat4(cam.getProjectionMatrix() * cam.getViewMatrix());
        cam.setFrontOrthoPreset();
        const bool frontOk = isFiniteMat4(cam.getProjectionMatrix() * cam.getViewMatrix());
        cam.reset();
        const bool resetOk = isFiniteMat4(cam.getProjectionMatrix() * cam.getViewMatrix()) &&
                             cam.getProjectionMode() == roomcraft::graphics::ProjectionMode::Perspective;
        check("Top Ortho, Front Ortho, and Reset presets valid", topOk && frontOk && resetOk);
    }

    return failures;
}
