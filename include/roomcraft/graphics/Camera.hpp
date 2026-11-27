#pragma once

#include "roomcraft/math/BoundingBox.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace roomcraft::graphics {

// Chế độ phép chiếu của Camera
enum class ProjectionMode {
    Perspective,
    Orthographic
};

// Lớp Camera quản lý ma trận View (glm::lookAt) và Projection (glm::perspective / glm::ortho)
// một cách độc lập, hỗ trợ Camera Auto-Fit theo BoundingBox (M3) và kiểm tra dữ liệu đầu vào hợp lệ.
class Camera {
public:
    Camera();

    bool lookAt(const glm::vec3& position,
                const glm::vec3& target,
                const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));

    bool setPosition(const glm::vec3& position);
    bool setTarget(const glm::vec3& target);
    bool setUp(const glm::vec3& up);

    void setProjectionMode(ProjectionMode mode) { m_projectionMode = mode; }
    bool setAspectRatio(float aspectRatio);
    bool setFieldOfView(float fovDegrees);
    bool setClipPlanes(float nearPlane, float farPlane);
    bool setOrthoHalfHeight(float halfHeight);

    // Tự động căn khung camera theo BoundingBox của mô hình (hỗ trợ cả Perspective và Orthographic,
    // xử lý aspect ratio ngang/dọc, kích thước nhỏ/lớn/cao/rộng và giữ nguyên hướng nhìn hiện tại)
    bool fitBoundingBox(const math::BoundingBox& box, float paddingFactor = 1.25f);

    void reset();
    void setPerspectivePreset();
    void setTopOrthoPreset();
    void setFrontOrthoPreset();

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;

    const glm::vec3& getPosition() const { return m_position; }
    const glm::vec3& getTarget() const { return m_target; }
    const glm::vec3& getUp() const { return m_up; }
    ProjectionMode getProjectionMode() const { return m_projectionMode; }
    float getAspectRatio() const { return m_aspectRatio; }
    float getFieldOfView() const { return m_fovDegrees; }
    float getNearPlane() const { return m_nearPlane; }
    float getFarPlane() const { return m_farPlane; }
    float getOrthoHalfHeight() const { return m_orthoHalfHeight; }

private:
    static bool isFiniteVec3(const glm::vec3& v);

private:
    glm::vec3 m_position{4.5f, 3.6f, 5.5f};
    glm::vec3 m_target{0.0f, 0.4f, 0.0f};
    glm::vec3 m_up{0.0f, 1.0f, 0.0f};

    ProjectionMode m_projectionMode = ProjectionMode::Perspective;
    float m_fovDegrees = 45.0f;
    float m_aspectRatio = 16.0f / 9.0f;
    float m_nearPlane = 0.05f;
    float m_farPlane = 100.0f;
    float m_orthoHalfHeight = 3.5f;
};

} // namespace roomcraft::graphics
