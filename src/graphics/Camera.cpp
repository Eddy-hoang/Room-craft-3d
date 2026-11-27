#include "roomcraft/graphics/Camera.hpp"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace roomcraft::graphics {

namespace {
constexpr float kMinDistance = 1e-4f;
constexpr float kMinNearPlane = 1e-4f;
constexpr float kMinFovDegrees = 5.0f;
constexpr float kMaxFovDegrees = 175.0f;
} // namespace

Camera::Camera()
{
    reset();
}

bool Camera::isFiniteVec3(const glm::vec3& v)
{
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

bool Camera::lookAt(const glm::vec3& position,
                    const glm::vec3& target,
                    const glm::vec3& up)
{
    if (!isFiniteVec3(position) || !isFiniteVec3(target) || !isFiniteVec3(up)) {
        return false;
    }

    const glm::vec3 forward = target - position;
    const float dist = glm::length(forward);
    if (dist < kMinDistance) {
        return false;
    }

    const float upLen = glm::length(up);
    if (upLen < kMinDistance) {
        return false;
    }

    const glm::vec3 forwardNorm = forward / dist;
    const glm::vec3 upNorm = up / upLen;
    if (glm::length(glm::cross(forwardNorm, upNorm)) < kMinDistance) {
        return false;
    }

    m_position = position;
    m_target = target;
    m_up = upNorm;
    return true;
}

bool Camera::setPosition(const glm::vec3& position)
{
    return lookAt(position, m_target, m_up);
}

bool Camera::setTarget(const glm::vec3& target)
{
    return lookAt(m_position, target, m_up);
}

bool Camera::setUp(const glm::vec3& up)
{
    return lookAt(m_position, m_target, up);
}

bool Camera::setAspectRatio(float aspectRatio)
{
    if (!std::isfinite(aspectRatio) || aspectRatio <= 1e-5f) {
        return false;
    }
    m_aspectRatio = aspectRatio;
    return true;
}

bool Camera::setFieldOfView(float fovDegrees)
{
    if (!std::isfinite(fovDegrees) || fovDegrees < kMinFovDegrees || fovDegrees > kMaxFovDegrees) {
        return false;
    }
    m_fovDegrees = fovDegrees;
    return true;
}

bool Camera::setClipPlanes(float nearPlane, float farPlane)
{
    if (!std::isfinite(nearPlane) || !std::isfinite(farPlane)) {
        return false;
    }
    if (nearPlane < kMinNearPlane || farPlane <= nearPlane + kMinNearPlane) {
        return false;
    }
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
    return true;
}

bool Camera::setOrthoHalfHeight(float halfHeight)
{
    if (!std::isfinite(halfHeight) || halfHeight <= 1e-4f) {
        return false;
    }
    m_orthoHalfHeight = halfHeight;
    return true;
}

bool Camera::fitBoundingBox(const math::BoundingBox& box, float paddingFactor)
{
    if (!box.isValid() || !std::isfinite(paddingFactor) || paddingFactor < 1.0f || paddingFactor > 5.0f) {
        return false;
    }

    const glm::vec3 center = box.getCenter();
    const glm::vec3 halfExtents = glm::max(box.getHalfExtents(), glm::vec3(0.02f));
    const float boundingRadius = glm::length(halfExtents);

    // Giữ nguyên hướng nhìn hiện tại từ target về position (nếu suy biến dùng hướng 3/4 mặc định)
    glm::vec3 viewOffset = m_position - m_target;
    float viewLen = glm::length(viewOffset);
    glm::vec3 viewDir = (viewLen > kMinDistance)
                            ? (viewOffset / viewLen)
                            : glm::normalize(glm::vec3(0.70f, 0.48f, 0.80f));

    // Xây dựng hệ cơ sở trực chuẩn (right, trueUp, viewDir) gắn với camera
    glm::vec3 refUp = m_up;
    if (glm::length(glm::cross(viewDir, refUp)) < kMinDistance) {
        refUp = (std::fabs(viewDir.y) > 0.95f) ? glm::vec3(0.0f, 0.0f, -1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    }
    const glm::vec3 right = glm::normalize(glm::cross(refUp, viewDir));
    const glm::vec3 trueUp = glm::normalize(glm::cross(viewDir, right));

    // Chiếu hình hộp bao lên 3 trục camera để tính bán kính bao khít theo ngang/dọc/sâu:
    //   projHalf(axis) = |axis.x|*hx + |axis.y|*hy + |axis.z|*hz
    const auto projectHalfExtent = [&halfExtents](const glm::vec3& axis) -> float {
        return std::fabs(axis.x) * halfExtents.x +
               std::fabs(axis.y) * halfExtents.y +
               std::fabs(axis.z) * halfExtents.z;
    };

    const float projHalfX = std::max(0.02f, projectHalfExtent(right));
    const float projHalfY = std::max(0.02f, projectHalfExtent(trueUp));
    const float projHalfZ = std::max(0.02f, projectHalfExtent(viewDir));

    const float safeAspect = std::max(0.05f, m_aspectRatio);

    if (m_projectionMode == ProjectionMode::Perspective) {
        const float halfFovY = glm::radians(m_fovDegrees) * 0.5f;
        const float tanHalfY = std::tan(halfFovY);
        const float tanHalfX = tanHalfY * safeAspect;

        // Khoảng cách tối thiểu để toàn bộ AABB lọt trong cả chiều đứng và chiều ngang frustum
        const float distY = (projHalfY * paddingFactor) / std::max(1e-4f, tanHalfY);
        const float distX = (projHalfX * paddingFactor) / std::max(1e-4f, tanHalfX);
        const float distance = std::max(distX, distY) + projHalfZ;

        m_target = center;
        m_position = center + viewDir * distance;
        m_up = trueUp;

        // Cập nhật near/far plane thích nghi với kích thước vật thể (tránh cắt mặt gần ở vật nhỏ)
        m_nearPlane = std::clamp(distance * 0.02f, 0.01f, 0.5f);
        m_farPlane = std::max(50.0f, distance + boundingRadius * 6.0f);
    } else {
        // Phép chiếu trực giao: chọn nửa chiều cao khung nhìn sao cho cả chiều ngang và dọc đều đủ chỗ
        const float requiredHalfH = std::max(projHalfY, projHalfX / safeAspect) * paddingFactor;
        m_orthoHalfHeight = std::max(0.05f, requiredHalfH);

        const float distance = std::max(2.0f, projHalfZ * 3.0f + boundingRadius * 2.0f);
        m_target = center;
        m_position = center + viewDir * distance;
        m_up = trueUp;

        m_nearPlane = 0.01f;
        m_farPlane = std::max(50.0f, distance + boundingRadius * 6.0f);
    }

    return true;
}

void Camera::reset()
{
    setPerspectivePreset();
}

void Camera::setPerspectivePreset()
{
    m_projectionMode = ProjectionMode::Perspective;
    m_fovDegrees = 45.0f;
    m_nearPlane = 0.05f;
    m_farPlane = 100.0f;
    m_orthoHalfHeight = 3.5f;
    lookAt(glm::vec3(4.5f, 3.6f, 5.5f),
           glm::vec3(0.0f, 0.4f, 0.0f),
           glm::vec3(0.0f, 1.0f, 0.0f));
}

void Camera::setTopOrthoPreset()
{
    m_projectionMode = ProjectionMode::Orthographic;
    m_nearPlane = 0.05f;
    m_farPlane = 100.0f;
    m_orthoHalfHeight = 4.0f;
    lookAt(glm::vec3(0.0f, 10.0f, 0.0f),
           glm::vec3(0.0f, 0.0f, 0.0f),
           glm::vec3(0.0f, 0.0f, -1.0f));
}

void Camera::setFrontOrthoPreset()
{
    m_projectionMode = ProjectionMode::Orthographic;
    m_nearPlane = 0.05f;
    m_farPlane = 100.0f;
    m_orthoHalfHeight = 3.0f;
    lookAt(glm::vec3(0.0f, 0.8f, 8.0f),
           glm::vec3(0.0f, 0.8f, 0.0f),
           glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(m_position, m_target, m_up);
}

glm::mat4 Camera::getProjectionMatrix() const
{
    if (m_projectionMode == ProjectionMode::Perspective) {
        return glm::perspective(glm::radians(m_fovDegrees), m_aspectRatio, m_nearPlane, m_farPlane);
    }

    const float halfWidth = m_orthoHalfHeight * m_aspectRatio;
    return glm::ortho(-halfWidth, halfWidth,
                      -m_orthoHalfHeight, m_orthoHalfHeight,
                      m_nearPlane, m_farPlane);
}

} // namespace roomcraft::graphics
