#include "roomcraft/graphics/Camera.hpp"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

    // Tránh trường hợp vector nhìn song song hoàn toàn với vector Up gây suy biến tích có hướng
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

void Camera::reset()
{
    setPerspectivePreset();
}

void Camera::setPerspectivePreset()
{
    m_projectionMode = ProjectionMode::Perspective;
    m_fovDegrees = 45.0f;
    m_nearPlane = 0.1f;
    m_farPlane = 100.0f;
    m_orthoHalfHeight = 3.5f;
    lookAt(glm::vec3(4.5f, 3.6f, 5.5f),
           glm::vec3(0.0f, 0.4f, 0.0f),
           glm::vec3(0.0f, 1.0f, 0.0f));
}

void Camera::setTopOrthoPreset()
{
    m_projectionMode = ProjectionMode::Orthographic;
    m_nearPlane = 0.1f;
    m_farPlane = 100.0f;
    m_orthoHalfHeight = 4.0f;
    // Khi nhìn thẳng xuống theo -Y, dùng -Z làm vector Up để tránh suy biến lookAt
    lookAt(glm::vec3(0.0f, 10.0f, 0.0f),
           glm::vec3(0.0f, 0.0f, 0.0f),
           glm::vec3(0.0f, 0.0f, -1.0f));
}

void Camera::setFrontOrthoPreset()
{
    m_projectionMode = ProjectionMode::Orthographic;
    m_nearPlane = 0.1f;
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
