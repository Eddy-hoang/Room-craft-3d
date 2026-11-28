#include "roomcraft/graphics/Camera.hpp"
#include "roomcraft/systems/CameraController.hpp"

#include <glm/geometric.hpp>

#include <cmath>
#include <iostream>

namespace {

bool approx(float a, float b, float eps = 1e-2f)
{
    return std::fabs(a - b) <= eps;
}

bool check(bool cond, const char* label, int& failures)
{
    std::cout << "  [CameraController] " << label << ": " << (cond ? "PASS" : "FAIL") << std::endl;
    if (!cond) ++failures;
    return cond;
}

} // namespace

int runCameraControllerTests()
{
    int failures = 0;

    roomcraft::math::BoundingBox roomBox;
    roomBox.expand({-2.65f, -0.10f, -3.15f});
    roomBox.expand({ 2.65f,  3.00f,  3.15f});

    roomcraft::graphics::Camera cam;
    cam.setAspectRatio(16.0f / 9.0f);
    roomcraft::systems::CameraController ctrl;
    ctrl.setPerspectivePreset(roomBox, cam);

    // TC-M5-19: Camera orbit changes yaw/pitch and moves camera on sphere around target at constant distance
    const float distBefore = ctrl.getDistance();
    const glm::vec3 targetBefore = ctrl.getTarget();
    const glm::vec3 posBefore = cam.getPosition();
    ctrl.orbit(60.0f, 20.0f, cam);
    const float distAfter = glm::length(cam.getPosition() - cam.getTarget());
    check(approx(distAfter, distBefore) &&
          glm::length(cam.getTarget() - targetBefore) < 1e-4f &&
          glm::length(cam.getPosition() - posBefore) > 0.5f,
          "TC-M5-19 Orbit rotates camera around target preserving radius", failures);

    // TC-M5-20: Pitch limits clamped to [-85, +85] degrees (no singularity)
    ctrl.orbit(0.0f, 5000.0f, cam);
    const bool maxClamped = ctrl.getPitchDegrees() <= ctrl.getMaxPitchDegrees() + 1e-3f &&
                            ctrl.getPitchDegrees() >= ctrl.getMaxPitchDegrees() - 1e-3f;
    ctrl.orbit(0.0f, -10000.0f, cam);
    const bool minClamped = ctrl.getPitchDegrees() >= ctrl.getMinPitchDegrees() - 1e-3f &&
                            ctrl.getPitchDegrees() <= ctrl.getMinPitchDegrees() + 1e-3f;
    check(maxClamped && minClamped && std::isfinite(cam.getViewMatrix()[0][0]),
          "TC-M5-20 Camera pitch safely clamped within [-85, +85] deg", failures);

    // TC-M5-21: Camera pan shifts position and target synchronously (preserves view vector)
    ctrl.setPerspectivePreset(roomBox, cam);
    const glm::vec3 p0 = cam.getPosition();
    const glm::vec3 t0 = cam.getTarget();
    ctrl.pan(120.0f, -60.0f, 1280.0f, 720.0f, cam);
    const glm::vec3 dp = cam.getPosition() - p0;
    const glm::vec3 dt = cam.getTarget() - t0;
    check(glm::length(dp) > 0.2f && glm::length(dp - dt) < 1e-3f,
          "TC-M5-21 Camera pan moves position and target synchronously", failures);

    // TC-M5-22: Perspective zoom adjusts orbit distance & respects min/max bounds
    const float d0 = ctrl.getDistance();
    ctrl.zoom(3.0f, cam);
    const float dCloser = ctrl.getDistance();
    ctrl.zoom(200.0f, cam);
    const float dMin = ctrl.getDistance();
    ctrl.zoom(-400.0f, cam);
    const float dMax = ctrl.getDistance();
    check(dCloser < d0 && approx(dMin, ctrl.getMinDistance()) && approx(dMax, ctrl.getMaxDistance()) &&
          cam.getNearPlane() > 0.0f && cam.getFarPlane() > cam.getNearPlane(),
          "TC-M5-22 Perspective zoom changes distance with safe min/max & clip planes", failures);

    // TC-M5-23: Orthographic zoom changes orthoHalfHeight instead of scaling scene
    ctrl.setTopViewPreset(roomBox, cam);
    const float h0 = cam.getOrthoHalfHeight();
    ctrl.zoom(4.0f, cam);
    const float hIn = cam.getOrthoHalfHeight();
    ctrl.zoom(-6.0f, cam);
    const float hOut = cam.getOrthoHalfHeight();
    check(cam.getProjectionMode() == roomcraft::graphics::ProjectionMode::Orthographic &&
          hIn < h0 && hOut > hIn,
          "TC-M5-23 Orthographic zoom adjusts projection half-height smoothly", failures);

    // TC-M5-24 & TC-M5-25: Preset & Focus sync (small orbit after Fit does NOT jump back to old position)
    roomcraft::math::BoundingBox deskBox;
    deskBox.expand({1.0f, 0.0f, -2.0f});
    deskBox.expand({2.4f, 0.75f, -1.3f});
    ctrl.setPerspectivePreset(roomBox, cam);
    const bool focused = ctrl.fitBoundingBox(deskBox, cam, 1.35f);
    const glm::vec3 posRightAfterFit = cam.getPosition();
    const glm::vec3 tgtRightAfterFit = cam.getTarget();
    ctrl.orbit(1.0f, 0.0f, cam);
    const float jumpDist = glm::length(cam.getPosition() - posRightAfterFit);
    check(focused && glm::length(tgtRightAfterFit - deskBox.getCenter()) < 1e-3f && jumpDist < 0.25f,
          "TC-M5-24/25 Camera preset & Focus Selected sync prevents orbit jump", failures);

    // RC2.3 P0 Regression: fitRoomView căn khung đối xứng sát biên không cắt phòng ở nhiều tỷ lệ khung hình
    {
        roomcraft::scene::Room room;
        const float aspects[3] = {16.0f / 9.0f, 4.0f / 3.0f, 676.0f / 550.0f};
        bool framingOk = true;
        for (float ar : aspects) {
            cam.setAspectRatio(ar);
            ctrl.setPerspectiveRoomPreset(room, cam);
            const glm::mat4 vp = cam.getProjectionMatrix() * cam.getViewMatrix();
            float minNdcX = 1e9f, maxNdcX = -1e9f;
            float minNdcY = 1e9f, maxNdcY = -1e9f;
            for (const auto& pt : room.collectVisibleCornerPoints()) {
                const glm::vec4 clip = vp * glm::vec4(pt, 1.0f);
                if (clip.w <= 0.0f) {
                    framingOk = false;
                    continue;
                }
                const glm::vec3 ndc = glm::vec3(clip) / clip.w;
                minNdcX = std::min(minNdcX, ndc.x);
                maxNdcX = std::max(maxNdcX, ndc.x);
                minNdcY = std::min(minNdcY, ndc.y);
                maxNdcY = std::max(maxNdcY, ndc.y);
            }
            const float maxAbsX = std::max(std::fabs(minNdcX), std::fabs(maxNdcX));
            const float maxAbsY = std::max(std::fabs(minNdcY), std::fabs(maxNdcY));
            const bool perspCentered = std::fabs(minNdcX + maxNdcX) < 0.10f && std::fabs(minNdcY + maxNdcY) < 0.10f;
            const bool perspOk = (maxAbsX <= 0.98f) && (maxAbsY <= 0.98f) &&
                                 (std::max(maxAbsX, maxAbsY) >= 0.88f) && perspCentered;
            if (!perspOk) {
                std::cout << "    [DIAG] ar=" << ar << " ndcX=[" << minNdcX << "," << maxNdcX
                          << "] ndcY=[" << minNdcY << "," << maxNdcY << "]" << std::endl;
            }
            framingOk &= perspOk;

            // Kiểm tra tương tự với Isometric Orthographic
            ctrl.setIsometricRoomPreset(room, cam);
            const glm::mat4 isoVp = cam.getProjectionMatrix() * cam.getViewMatrix();
            float isoMinX = 1e9f, isoMaxX = -1e9f;
            float isoMinY = 1e9f, isoMaxY = -1e9f;
            for (const auto& pt : room.collectVisibleCornerPoints()) {
                const glm::vec4 clip = isoVp * glm::vec4(pt, 1.0f);
                isoMinX = std::min(isoMinX, clip.x);
                isoMaxX = std::max(isoMaxX, clip.x);
                isoMinY = std::min(isoMinY, clip.y);
                isoMaxY = std::max(isoMaxY, clip.y);
            }
            const float isoAbsX = std::max(std::fabs(isoMinX), std::fabs(isoMaxX));
            const float isoAbsY = std::max(std::fabs(isoMinY), std::fabs(isoMaxY));
            const bool isoOk = (isoAbsX <= 0.98f) && (isoAbsY <= 0.98f) &&
                               (std::max(isoAbsX, isoAbsY) >= 0.88f) &&
                               std::fabs(isoMinX + isoMaxX) < 0.05f && std::fabs(isoMinY + isoMaxY) < 0.05f;
            if (!isoOk) {
                std::cout << "    [DIAG] ISO ar=" << ar << " isoX=[" << isoMinX << "," << isoMaxX
                          << "] isoY=[" << isoMinY << "," << isoMaxY << "]" << std::endl;
            }
            framingOk &= isoOk;
        }
        check(framingOk, "RC2-P0-CAM fitRoomView frames visible room tightly without clipping across aspect ratios", failures);
    }

    return failures;
}
