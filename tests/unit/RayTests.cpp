#include "roomcraft/graphics/Camera.hpp"
#include "roomcraft/math/Ray.hpp"

#include <glm/geometric.hpp>

#include <cmath>
#include <iostream>
#include <limits>

namespace {

bool approx(float a, float b, float eps = 1e-3f)
{
    return std::fabs(a - b) <= eps;
}

bool check(bool cond, const char* label, int& failures)
{
    std::cout << "  [Ray] " << label << ": " << (cond ? "PASS" : "FAIL") << std::endl;
    if (!cond) ++failures;
    return cond;
}

} // namespace

int runRayTests()
{
    int failures = 0;

    roomcraft::math::ViewportRect vp;
    vp.minX = 200.0f;
    vp.minY = 100.0f;
    vp.maxX = 1000.0f;  // display width = 800
    vp.maxY = 700.0f;   // display height = 600
    vp.framebufferWidth = 1600.0f;  // 2x DPI scaling
    vp.framebufferHeight = 1200.0f;

    // TC-M5-02 & TC-M5-03: Mouse coordinate conversion & viewport-local mapping + NDC Y flip
    glm::vec2 local(0.0f), ndcCenter(0.0f), ndcTopLeft(0.0f), ndcBotRight(0.0f);
    const bool localOk = vp.mapScreenToViewportLocal(600.0f, 400.0f, local) &&
                         approx(local.x, 400.0f) && approx(local.y, 300.0f);
    const bool ndcOk = vp.mapScreenToNdc(600.0f, 400.0f, ndcCenter) && approx(ndcCenter.x, 0.0f) && approx(ndcCenter.y, 0.0f) &&
                       vp.mapScreenToNdc(200.0f, 100.0f, ndcTopLeft) && approx(ndcTopLeft.x, -1.0f) && approx(ndcTopLeft.y, 1.0f) &&
                       vp.mapScreenToNdc(1000.0f, 700.0f, ndcBotRight) && approx(ndcBotRight.x, 1.0f) && approx(ndcBotRight.y, -1.0f);
    check(localOk && ndcOk, "TC-M5-02/03 Screen -> Viewport-local -> OpenGL NDC mapping (with Y flip)", failures);

    // TC-M5-04: Framebuffer / display size scaling (2x DPI)
    glm::vec2 fbPos(0.0f);
    const bool dpiOk = vp.mapScreenToFramebuffer(600.0f, 400.0f, fbPos) &&
                       approx(fbPos.x, 800.0f) && approx(fbPos.y, 600.0f);
    check(dpiOk, "TC-M5-04 Framebuffer vs display size (2x DPI) coordinate scaling", failures);

    // TC-M5-05: Perspective ray generation (center ray points straight along camera forward -Z; off-center rays diverge)
    roomcraft::graphics::Camera perspCam;
    perspCam.setProjectionMode(roomcraft::graphics::ProjectionMode::Perspective);
    perspCam.setAspectRatio(800.0f / 600.0f);
    perspCam.lookAt({0.0f, 2.0f, 5.0f}, {0.0f, 2.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

    roomcraft::math::Ray rCenter, rRight;
    const bool pOk = roomcraft::math::Ray::screenToWorldRay(600.0f, 400.0f, vp, perspCam.getViewMatrix(), perspCam.getProjectionMatrix(), rCenter) &&
                     roomcraft::math::Ray::screenToWorldRay(900.0f, 400.0f, vp, perspCam.getViewMatrix(), perspCam.getProjectionMatrix(), rRight);
    check(pOk && approx(rCenter.direction.x, 0.0f) && approx(rCenter.direction.y, 0.0f) && approx(rCenter.direction.z, -1.0f) &&
          rRight.direction.x > 0.1f && approx(glm::length(rRight.direction), 1.0f),
          "TC-M5-05/08 Perspective ray generation & unit-length normalization", failures);

    // TC-M5-06: Orthographic ray generation (origins vary with mouse pos, directions are parallel!)
    roomcraft::graphics::Camera orthoCam;
    orthoCam.setProjectionMode(roomcraft::graphics::ProjectionMode::Orthographic);
    orthoCam.setAspectRatio(800.0f / 600.0f);
    orthoCam.setOrthoHalfHeight(3.0f);
    orthoCam.lookAt({0.0f, 10.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f});

    roomcraft::math::Ray oLeft, oRight;
    const bool oOk = roomcraft::math::Ray::screenToWorldRay(300.0f, 400.0f, vp, orthoCam.getViewMatrix(), orthoCam.getProjectionMatrix(), oLeft) &&
                     roomcraft::math::Ray::screenToWorldRay(900.0f, 400.0f, vp, orthoCam.getViewMatrix(), orthoCam.getProjectionMatrix(), oRight);
    check(oOk && approx(oLeft.direction.y, -1.0f) && approx(oRight.direction.y, -1.0f) &&
          approx(glm::dot(oLeft.direction, oRight.direction), 1.0f) &&
          (oRight.origin.x - oLeft.origin.x > 4.0f),
          "TC-M5-06 Orthographic ray generation: parallel directions & distinct world origins", failures);

    // TC-M5-07: Invalid viewport / outside viewport / singular matrix / zero direction rejected safely
    roomcraft::math::ViewportRect zeroVp;
    roomcraft::math::Ray badRay;
    const bool rejected =
        !roomcraft::math::Ray::screenToWorldRay(10.0f, 10.0f, zeroVp, perspCam.getViewMatrix(), perspCam.getProjectionMatrix(), badRay) &&
        !roomcraft::math::Ray::screenToWorldRay(50.0f, 50.0f, vp, perspCam.getViewMatrix(), perspCam.getProjectionMatrix(), badRay, true) &&
        !roomcraft::math::Ray::screenToWorldRay(600.0f, 400.0f, vp, glm::mat4(0.0f), perspCam.getProjectionMatrix(), badRay) &&
        !roomcraft::math::Ray::create({0, 0, 0}, {0, 0, 0}, badRay) &&
        !roomcraft::math::Ray::create({std::numeric_limits<float>::quiet_NaN(), 0, 0}, {0, 0, -1}, badRay);
    check(rejected, "TC-M5-07 Invalid viewport, out-of-bounds cursor, singular VP & zero/NaN ray rejected", failures);

    return failures;
}
