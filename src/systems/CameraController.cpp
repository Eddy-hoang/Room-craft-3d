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
    camera.lookAt(glm::vec3(5.4f, 4.1f, 5.8f), glm::vec3(0.0f, 0.95f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    if (roomBounds.isValid()) {
        camera.fitBoundingBox(roomBounds, 0.94f);
    }
    syncFromCamera(camera);
}

void CameraController::setIsometricPreset(const math::BoundingBox& roomBounds, graphics::Camera& camera)
{
    camera.setProjectionMode(graphics::ProjectionMode::Orthographic);
    camera.lookAt(glm::vec3(7.0f, 5.8f, 7.0f), glm::vec3(0.0f, 0.95f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    if (roomBounds.isValid()) {
        camera.fitBoundingBox(roomBounds, 0.96f);
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

bool CameraController::fitRoomView(const scene::Room& room,
                                   graphics::Camera& camera,
                                   float marginFactor)
{
    const auto pts = room.collectVisibleCornerPoints();
    if (pts.empty()) {
        return fitBoundingBox(room.getExteriorBounds(), camera, marginFactor);
    }

    const float safeMargin = std::clamp(marginFactor, 1.01f, 2.0f);
    glm::vec3 viewDir = camera.getPosition() - camera.getTarget();
    const float viewLen = glm::length(viewDir);
    if (!std::isfinite(viewLen) || viewLen < 1e-4f) {
        viewDir = glm::normalize(glm::vec3(5.6f, 4.4f, 6.2f));
    } else {
        viewDir /= viewLen;
    }

    glm::vec3 refUp(0.0f, 1.0f, 0.0f);
    if (std::fabs(viewDir.y) > 0.95f) {
        refUp = glm::vec3(0.0f, 0.0f, -1.0f);
    }
    const glm::vec3 right = glm::normalize(glm::cross(refUp, viewDir));
    const glm::vec3 trueUp = glm::normalize(glm::cross(viewDir, right));

    // Tính tâm nhìn dựa trên trung điểm min/max chiếu lên mặt phẳng camera của các đỉnh ĐANG HIỂN THỊ
    // (loại bỏ góc không khí rỗng phía Đông-Nam của CutawayView -> khung hình cân đối và đầy đặn 52-62%)
    float minX = 1e9f, maxX = -1e9f;
    float minY = 1e9f, maxY = -1e9f;
    float minZ = 1e9f, maxZ = -1e9f;
    for (const auto& p : pts) {
        const float px = glm::dot(p, right);
        const float py = glm::dot(p, trueUp);
        const float pz = glm::dot(p, viewDir);
        minX = std::min(minX, px);
        maxX = std::max(maxX, px);
        minY = std::min(minY, py);
        maxY = std::max(maxY, py);
        minZ = std::min(minZ, pz);
        maxZ = std::max(maxZ, pz);
    }

    glm::vec3 target = right * (0.5f * (minX + maxX))
                     + trueUp * (0.5f * (minY + maxY))
                     + viewDir * (0.5f * (minZ + maxZ));
    const float safeAspect = std::max(0.05f, camera.getAspectRatio());

    if (camera.getProjectionMode() == graphics::ProjectionMode::Perspective) {
        const float halfFovY = glm::radians(camera.getFieldOfView()) * 0.5f;
        const float tanHalfY = std::max(1e-4f, std::tan(halfFovY));
        const float tanHalfX = tanHalfY * safeAspect;

        auto solveDistanceForTarget = [&](const glm::vec3& tgt) -> float {
            float d = 1.0f;
            for (const auto& p : pts) {
                const glm::vec3 rel = p - tgt;
                const float x = std::fabs(glm::dot(rel, right));
                const float y = std::fabs(glm::dot(rel, trueUp));
                const float z = glm::dot(rel, viewDir);
                d = std::max(d, z + (x * safeMargin) / tanHalfX);
                d = std::max(d, z + (y * safeMargin) / tanHalfY);
            }
            return std::clamp(d, m_minDistance, m_maxDistance);
        };

        // Lặp 3 bước cân bằng tâm chiếu phối cảnh (NDC center -> 0) để đỉnh tường trên và góc sàn dưới
        // cách đều 2 mép viewport, loại bỏ khoảng trống thừa trên đỉnh và đạt độ phủ 50–62% không cắt phòng.
        float dist = solveDistanceForTarget(target);
        for (int iter = 0; iter < 3; ++iter) {
            float ndcMinX = 1e9f, ndcMaxX = -1e9f;
            float ndcMinY = 1e9f, ndcMaxY = -1e9f;
            for (const auto& p : pts) {
                const glm::vec3 rel = p - target;
                const float depth = std::max(0.1f, dist - glm::dot(rel, viewDir));
                const float nx = glm::dot(rel, right) / (depth * tanHalfX);
                const float ny = glm::dot(rel, trueUp) / (depth * tanHalfY);
                ndcMinX = std::min(ndcMinX, nx);
                ndcMaxX = std::max(ndcMaxX, nx);
                ndcMinY = std::min(ndcMinY, ny);
                ndcMaxY = std::max(ndcMaxY, ny);
            }
            const float midNdcX = 0.5f * (ndcMinX + ndcMaxX);
            const float midNdcY = 0.5f * (ndcMinY + ndcMaxY);
            target += right * (midNdcX * dist * tanHalfX * 0.85f)
                    + trueUp * (midNdcY * dist * tanHalfY * 0.85f);
            dist = solveDistanceForTarget(target);
        }

        // Sau bước dịch target cuối, kiểm tra biên thực tế 1 lần nữa để đảm bảo mọi đỉnh <= 1/safeMargin
        float maxRatio = 0.0f;
        for (const auto& p : pts) {
            const glm::vec3 rel = p - target;
            const float depth = std::max(0.1f, dist - glm::dot(rel, viewDir));
            maxRatio = std::max(maxRatio, std::fabs(glm::dot(rel, right)) / (depth * tanHalfX));
            maxRatio = std::max(maxRatio, std::fabs(glm::dot(rel, trueUp)) / (depth * tanHalfY));
        }
        const float targetMaxNdc = 1.0f / safeMargin;
        if (maxRatio > targetMaxNdc && maxRatio > 1e-4f) {
            dist = std::clamp(dist * (maxRatio / targetMaxNdc), m_minDistance, m_maxDistance);
        }

        camera.lookAt(target + viewDir * dist, target, trueUp);
        const float radius = std::max(1.0f, 0.5f * std::hypot(maxX - minX, std::hypot(maxY - minY, maxZ - minZ)));
        camera.setClipPlanes(std::clamp(dist * 0.015f, 0.02f, 0.40f),
                             std::max(60.0f, dist + radius * 6.0f));
    } else {
        const float halfSpanX = 0.5f * (maxX - minX);
        const float halfSpanY = 0.5f * (maxY - minY);
        const float halfSpanZ = 0.5f * (maxZ - minZ);
        const float reqHalfH = std::max(halfSpanY, halfSpanX / safeAspect) * safeMargin;
        camera.setOrthoHalfHeight(std::clamp(reqHalfH, m_minOrthoHalfHeight, m_maxOrthoHalfHeight));
        const float dist = std::max(4.0f, halfSpanZ * 3.5f + std::max(halfSpanX, halfSpanY) * 2.0f);
        camera.lookAt(target + viewDir * dist, target, trueUp);
        camera.setClipPlanes(0.01f, std::max(60.0f, dist * 4.0f));
    }

    syncFromCamera(camera);
    return true;
}

void CameraController::setPerspectiveRoomPreset(const scene::Room& room, graphics::Camera& camera)
{
    camera.setProjectionMode(graphics::ProjectionMode::Perspective);
    // Góc nhìn 3/4 ấm áp, cân đối mặt sàn và hai tường Cutaway với biên thoáng studio
    camera.lookAt(glm::vec3(5.8f, 4.5f, 6.4f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    fitRoomView(room, camera, 1.11f);
}

void CameraController::setIsometricRoomPreset(const scene::Room& room, graphics::Camera& camera)
{
    camera.setProjectionMode(graphics::ProjectionMode::Orthographic);
    camera.lookAt(glm::vec3(7.0f, 5.8f, 7.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    fitRoomView(room, camera, 1.09f);
}

void CameraController::reset(const math::BoundingBox& roomBounds, graphics::Camera& camera)
{
    setPerspectivePreset(roomBounds, camera);
}

} // namespace roomcraft::systems
