#pragma once

#include "roomcraft/graphics/Camera.hpp"
#include "roomcraft/math/BoundingBox.hpp"
#include "roomcraft/scene/Room.hpp"

#include <glm/vec3.hpp>
#include <vector>

namespace roomcraft::systems {

// Bộ điều khiển Camera tương tác (Orbit / Pan / Zoom / Focus / Presets) thuần CPU.
// Sử dụng tọa độ cầu quanh target (Right-handed, Y-up, mét) và luôn đồng bộ trạng thái
// khi chuyển preset hoặc Fit BoundingBox để không bao giờ bị nhảy camera khi bắt đầu Orbit.
class CameraController {
public:
    CameraController() = default;

    // Đồng bộ target, distance, yawDegrees, pitchDegrees từ trạng thái hiện tại của Camera
    void syncFromCamera(const graphics::Camera& camera);

    // Áp dụng tọa độ cầu hiện tại lên Camera (không làm thay đổi SceneObject)
    void applyToCamera(graphics::Camera& camera) const;

    // Xoay camera quanh target (chuột phải kéo): cập nhật yaw và pitch (clamp [-85, +85] độ)
    void orbit(float deltaX, float deltaY, graphics::Camera& camera);

    // Dịch chuyển song song camera và target theo right / trueUp (chuột giữa kéo)
    void pan(float deltaX,
             float deltaY,
             float viewportWidth,
             float viewportHeight,
             graphics::Camera& camera);

    // Thu phóng bằng con lăn chuột:
    // - Perspective: thay đổi khoảng cách r (clamp [minDistance, maxDistance])
    // - Orthographic: thay đổi orthoHalfHeight (clamp [minOrthoHalfHeight, maxOrthoHalfHeight])
    void zoom(float wheelDelta, graphics::Camera& camera);

    // Presets & Focus (tự động gọi syncFromCamera sau khi thay đổi góc nhìn)
    void setPerspectivePreset(const math::BoundingBox& roomBounds, graphics::Camera& camera);
    void setIsometricPreset(const math::BoundingBox& roomBounds, graphics::Camera& camera);
    void setTopViewPreset(const math::BoundingBox& roomBounds, graphics::Camera& camera);
    void setFrontViewPreset(const math::BoundingBox& roomBounds, graphics::Camera& camera);
    bool fitBoundingBox(const math::BoundingBox& bounds, graphics::Camera& camera, float paddingFactor = 1.25f);

    // RC2.3: Căn khung chính xác theo đỉnh thực tế của các bề mặt phòng đang hiển thị (không cắt phòng, ~52-62% viewport)
    bool fitRoomView(const scene::Room& room, graphics::Camera& camera, float marginFactor = 1.07f);
    void setPerspectiveRoomPreset(const scene::Room& room, graphics::Camera& camera);
    void setIsometricRoomPreset(const scene::Room& room, graphics::Camera& camera);

    void reset(const math::BoundingBox& roomBounds, graphics::Camera& camera);

    // Getters / Setters phục vụ unit tests & Inspector
    const glm::vec3& getTarget() const { return m_target; }
    float getDistance() const { return m_distance; }
    float getYawDegrees() const { return m_yawDegrees; }
    float getPitchDegrees() const { return m_pitchDegrees; }
    float getMinDistance() const { return m_minDistance; }
    float getMaxDistance() const { return m_maxDistance; }
    float getMinPitchDegrees() const { return m_minPitchDegrees; }
    float getMaxPitchDegrees() const { return m_maxPitchDegrees; }

    void setOrbitSensitivity(float degPerPixel) { m_orbitSensitivity = degPerPixel; }
    void setPanSensitivity(float scale) { m_panSensitivity = scale; }
    void setZoomSensitivity(float factor) { m_zoomSensitivity = factor; }

private:
    glm::vec3 m_target{0.0f, 1.0f, 0.0f};
    float m_distance = 9.0f;
    float m_yawDegrees = 48.0f;
    float m_pitchDegrees = 32.0f;

    float m_orbitSensitivity = 0.35f;   // độ / pixel
    float m_panSensitivity = 1.0f;
    float m_zoomSensitivity = 0.12f;    // tỷ lệ thu phóng mỗi nấc cuộn

    float m_minDistance = 0.50f;
    float m_maxDistance = 60.0f;
    float m_minPitchDegrees = -85.0f;
    float m_maxPitchDegrees = 85.0f;
    float m_minOrthoHalfHeight = 0.30f;
    float m_maxOrthoHalfHeight = 40.0f;
};

} // namespace roomcraft::systems
