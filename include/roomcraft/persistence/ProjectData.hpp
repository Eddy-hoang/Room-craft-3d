#pragma once

#include "roomcraft/graphics/Camera.hpp"
#include "roomcraft/graphics/Lighting.hpp"
#include "roomcraft/scene/ObjectId.hpp"
#include "roomcraft/scene/RoomSettings.hpp"
#include "roomcraft/scene/SceneObject.hpp"
#include "roomcraft/systems/CollisionSystem.hpp"

#include <glm/vec3.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace roomcraft::persistence {

// Phiên bản schema tệp dự án (.roomcraft) — ĐỘC LẬP với phiên bản ứng dụng (v0.8.0).
inline constexpr const char* kProjectFormatId = "RoomCraft3D";
inline constexpr int kCurrentSchemaVersion = 1;
inline constexpr std::size_t kMaxProjectObjects = 5000;

struct ProjectCameraData {
    glm::vec3 position{4.5f, 3.6f, 5.5f};
    glm::vec3 target{0.0f, 0.4f, 0.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    graphics::ProjectionMode projectionMode = graphics::ProjectionMode::Perspective;
    float fovDegrees = 45.0f;
    float nearPlane = 0.05f;
    float farPlane = 100.0f;
    float orthoHalfHeight = 3.5f;

    static ProjectCameraData fromCamera(const graphics::Camera& cam)
    {
        ProjectCameraData d;
        d.position = cam.getPosition();
        d.target = cam.getTarget();
        d.up = cam.getUp();
        d.projectionMode = cam.getProjectionMode();
        d.fovDegrees = cam.getFieldOfView();
        d.nearPlane = cam.getNearPlane();
        d.farPlane = cam.getFarPlane();
        d.orthoHalfHeight = cam.getOrthoHalfHeight();
        return d;
    }

    bool applyToCamera(graphics::Camera& cam) const
    {
        cam.setProjectionMode(projectionMode);
        return cam.setFieldOfView(fovDegrees) &&
               cam.setClipPlanes(nearPlane, farPlane) &&
               cam.setOrthoHalfHeight(orthoHalfHeight) &&
               cam.lookAt(position, target, up);
    }
};

struct ProjectRenderSettingsData {
    int msaaSampleIndex = 0; // 0=Off, 1=2x, 2=4x
    bool wireframe = false;
    bool showAxes = true;
    bool showSelectedBBox = true;
    bool showMoveGizmo = true;
};

struct ProjectEditorSettingsData {
    bool showGrid = true;
    bool gridSnapEnabled = false;
    systems::PlacementValidationMode placementMode = systems::PlacementValidationMode::WarningOnly;
    bool allowRugOverlap = true;
};

// Dữ liệu dự án thuần CPU có thể serialize/deserialize hoàn toàn sang JSON (.roomcraft).
// TUYỆT ĐỐI KHÔNG chứa OpenGL handle (VAO/VBO/EBO/Texture ID), con trỏ GPU hay đường dẫn tuyệt đối.
struct ProjectData {
    std::string format = kProjectFormatId;
    int schemaVersion = kCurrentSchemaVersion;
    std::string projectName = "Untitled Room";

    scene::RoomSettings room;
    scene::ObjectId nextObjectId = 1;
    std::vector<scene::SceneObject> objects;

    // Cấu hình ánh sáng M7 đầy đủ (preset + mọi tham số tùy chỉnh)
    int lightingPreset = 0; // 0=Daylight, 1=WarmInterior, 2=NeutralStudio, 3=Evening, -1=Custom
    graphics::LightingSettings lighting;

    ProjectRenderSettingsData renderSettings;
    ProjectCameraData camera;
    ProjectEditorSettingsData editorSettings;
};

} // namespace roomcraft::persistence
