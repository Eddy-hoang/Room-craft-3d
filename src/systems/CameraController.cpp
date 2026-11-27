#include "roomcraft/systems/CameraController.hpp"

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>
#include <cmath>

namespace roomcraft::systems {

void CameraController::syncFromCamera(const graphics::Camera& camera)
{
    m_target = camera.getTarget();
    const glm::vec3 offset = camera.getPosition() - m_target;
    const float dist = glm::length(offset);
    if (!std::isfinite(dist) || dist < 1e-4f) {
        return;
    }

    m_distance = std::clamp(dist, m_minDistance, m_maxDistance);
    const glm::vec3 dir = offset / dist;

    // Nghịch đảo hệ tọa độ cầu:
    //   x = r * cos(pitch) * cos(yaw)
    //   y = r * sin(pitch)
    //   z = r * cos(pitch) * sin(yaw)
    const float clampedY = std::clamp(dir.y, -1.0f, 1.0f);
    m_pitchDegrees = glm::degrees(std::asin(clampedY));

    const float horizLen = std::hypot(dir.x, dir.z);
    if (horizLen > 1e-4f) {
        m_yawDegrees = glm::degrees(std::atan2(dir.z, dir.x));
    }
}

void CameraController::applyToCamera(graphics::Camera& camera) const
{
    const float yawRad = glm::radians(m_yawDegrees);
    const float pitchRad = glm::radians(std::clamp(m_pitchDegrees, m_minPitchDegrees, m_maxPitchDegrees));
    const float r = std::clamp(m_distance, m_minDistance, m_maxDistance);

    const float cosPitch = std::cos(pitchRad);
    const glm::vec3 offset(
        r * cosPitch * std::cos(yawRad),
        r * std::sin(pitchRad),
        r * cosPitch * std::sin(yawRad)
    );

    const glm::vec3 pos = m_target + offset;
    camera.lookAt(pos, m_target, glm::vec3(0.0f, 1.0f, 0.0f));

    // Giữ mặt phẳng cắt near/far hợp lệ theo khoảng cách hiện tại
    const float nearPlane = std::clamp(r * 0.01f, 0.02f, 0.50f);
    const float farPlane = std::max(100.0f, r * 8.0f);
    camera.setClipPlanes(nearPlane, farPlane);
}

void CameraController::orbit(float deltaX, float deltaY, graphics::Camera& camera)
{
    if (!std::isfinite(deltaX) || !std::isfinite(deltaY)) {
        return;
    }

    // Nếu đang ở Top View gần 90 độ, khi người dùng chủ động Orbit sẽ kẹp pitch xuống <= 85° mượt mà
    m_yawDegrees += deltaX * m_orbitSensitivity;
    if (m_yawDegrees > 180.0f || m_yawDegrees < -180.0f) {
        m_yawDegrees = std::fmod(m_yawDegrees + 180.0f, 360.0f);
        if (m_yawDegrees < 0.0f) {
            m_yawDegrees += 360.0f;
        }
        m_yawDegrees -= 180.0f;
    }

    m_pitchDegrees = std::clamp(m_pitchDegrees + deltaY * m_orbitSensitivity,
                                m_minPitchDegrees,
                                m_maxPitchDegrees);

    applyToCamera(camera);
}

void CameraController::pan(float deltaX,
                           float deltaY,
                           float viewportWidth,
                           float viewportHeight,
                           graphics::Camera& camera)
{
    if (!std::isfinite(deltaX) || !std::isfinite(deltaY) ||
        viewportWidth <= 1.0f || viewportHeight <= 1.0f) {
        return;
    }

    const glm::vec3 pos = camera.getPosition();
    const glm::vec3 tgt = camera.getTarget();
    const glm::vec3 forward = glm::normalize(tgt - pos);
    glm::vec3 upRef = camera.getUp();
    if (glm::length(glm::cross(forward, upRef)) < 1e-4f) {
        upRef = glm::vec3(0.0f, 0.0f, -1.0f);
    }

    const glm::vec3 right = glm::normalize(glm::cross(forward, upRef));
    const glm::vec3 trueUp = glm::normalize(glm::cross(right, forward));

    // Quy đổi tốc độ pan tỷ lệ thuận với kích thước thế giới hiển thị trên 1 pixel màn hình
    float worldHeightPerViewport = 2.0f * camera.getOrthoHalfHeight();
    if (camera.getProjectionMode() == graphics::ProjectionMode::Perspective) {
        const float halfFovRad = glm::radians(camera.getFieldOfView()) * 0.5f;
        worldHeightPerViewport = 2.0f * m_distance * std::tan(halfFovRad);
    }

    const float unitsPerPixel = (worldHeightPerViewport / viewportHeight) * m_panSensitivity;
    // Kéo chuột sang phải -> cảnh trượt sang phải -> camera dịch sang trái (-right)
    // Kéo chuột xuống dưới (+deltaY) -> cảnh trượt xuống -> camera dịch lên (+trueUp)
    const glm::vec3 shift = (-right * deltaX + trueUp * deltaY) * unitsPerPixel;
    if (!std::isfinite(shift.x) || !std::isfinite(shift.y) || !std::isfinite(shift.z)) {
        return;
    }

    m_target += shift;
    camera.lookAt(pos + shift, tgt + shift, camera.getUp());
}

void CameraController::zoom(float wheelDelta, graphics::Camera& camera)
{
    if (!std::isfinite(wheelDelta) || std::fabs(wheelDelta) < 1e-6f) {
        return;
    }

    // wheelDelta > 0 (cuộn lên) -> zoom vào gần (giảm distance / giảm orthoHalfHeight)
    const float factor = std::pow(1.0f - m_zoomSensitivity, wheelDelta);
    if (!std::isfinite(factor) || factor <= 0.0f) {
        return;
    }

    if (camera.getProjectionMode() == graphics::ProjectionMode::Perspective) {
        m_distance = std::clamp(m_distance * factor, m_minDistance, m_maxDistance);
        applyToCamera(camera);
    } else {
        const float nextHalfH = std::clamp(camera.getOrthoHalfHeight() * factor,
                                           m_minOrthoHalfHeight,
                                           m_maxOrthoHalfHeight);
        camera.setOrthoHalfHeight(nextHalfH);
    }
}

void CameraController::setPerspectivePreset(const math::BoundingBox& roomBounds, graphics::Camera& camera)
{
    camera.setProjectionMode(graphics::ProjectionMode::Perspective);
    camera.lookAt(glm::vec3(5.5f, 4.8f, 6.2f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    if (roomBounds.isValid()) {
        camera.fitBoundingBox(roomBounds, 1.22f);
    }
    syncFromCamera(camera);
}

void CameraController::setIsometricPreset(const math::BoundingBox& roomBounds, graphics::Camera& camera)
{
    camera.setProjectionMode(graphics::ProjectionMode::Orthographic);
    camera.lookAt(glm::vec3(7.0f, 6.2f, 7.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    if (roomBounds.isValid()) {
        camera.fitBoundingBox(roomBounds, 1.22f);
    }
    syncFromCamera(camera);
}

void CameraController::setTopViewPreset(const math::BoundingBox& roomBounds, graphics::Camera& camera)
{
    camera.setProjectionMode(graphics::ProjectionMode::Orthographic);
    // Dùng up = (0, 0, -1) để không song song với hướng nhìn thẳng đứng (0, -1, 0)
    camera.lookAt(glm::vec3(0.0f, 14.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    if (roomBounds.isValid()) {
        camera.fitBoundingBox(roomBounds, 1.18f);
    }
    syncFromCamera(camera);
}

void CameraController::setFrontViewPreset(const math::BoundingBox& roomBounds, graphics::Camera& camera)
{
    camera.setProjectionMode(graphics::ProjectionMode::Orthographic);
    const float midY = roomBounds.isValid() ? roomBounds.getCenter().y : 1.5f;
    camera.lookAt(glm::vec3(0.0f, midY, 12.0f), glm::vec3(0.0f, midY, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    if (roomBounds.isValid()) {
        camera.fitBoundingBox(roomBounds, 1.22f);
    }
    syncFromCamera(camera);
}

bool CameraController::fitBoundingBox(const math::BoundingBox& bounds,
                                      graphics::Camera& camera,
                                      float paddingFactor)
{
    if (!camera.fitBoundingBox(bounds, paddingFactor)) {
        return false;
    }
    syncFromCamera(camera);
    return true;
}

void CameraController::reset(const math::BoundingBox& roomBounds, graphics::Camera& camera)
{
    setPerspectivePreset(roomBounds, camera);
}

} // namespace roomcraft::systems
