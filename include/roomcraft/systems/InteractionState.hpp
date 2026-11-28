#pragma once

#include "roomcraft/scene/ObjectId.hpp"
#include "roomcraft/scene/Scene.hpp"
#include "roomcraft/scene/Transform.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <optional>
#include <string>

namespace roomcraft::systems {

// Chế độ công cụ thao tác trong Room Editor (Toolbar & phím tắt Q/G/R/S)
enum class TransformMode {
    Select = 0,
    Move = 1,
    Rotate = 2,
    Scale = 3
};

// Máy trạng thái tương tác (Interaction State Machine):
// Chỉ cho phép ĐÚNG MỘT tương tác hoạt động tại một thời điểm (không thể vừa xoay camera vừa kéo vật thể).
enum class InteractionPhase {
    Idle = 0,
    Selecting = 1,
    Orbiting = 2,
    Panning = 3,
    Moving = 4,
    Rotating = 5,
    Scaling = 6
};

// Giới hạn trục khi biến đổi đối tượng (phím X / Y / Z)
enum class AxisConstraint {
    None = 0,
    X = 1,
    Y = 2,
    Z = 3
};

inline const char* transformModeToString(TransformMode mode)
{
    switch (mode) {
        case TransformMode::Select: return "Select";
        case TransformMode::Move:   return "Move (G)";
        case TransformMode::Rotate: return "Rotate (R)";
        case TransformMode::Scale:  return "Scale (S)";
        default:                    return "Unknown";
    }
}

inline const char* interactionPhaseToString(InteractionPhase phase)
{
    switch (phase) {
        case InteractionPhase::Idle:      return "Idle";
        case InteractionPhase::Selecting: return "Selecting";
        case InteractionPhase::Orbiting:  return "Orbiting";
        case InteractionPhase::Panning:   return "Panning";
        case InteractionPhase::Moving:    return "Moving";
        case InteractionPhase::Rotating:  return "Rotating";
        case InteractionPhase::Scaling:   return "Scaling";
        default:                          return "Unknown";
    }
}

inline const char* axisConstraintToString(AxisConstraint axis)
{
    switch (axis) {
        case AxisConstraint::None: return "Free";
        case AxisConstraint::X:    return "Axis X";
        case AxisConstraint::Y:    return "Axis Y";
        case AxisConstraint::Z:    return "Axis Z";
        default:                   return "Free";
    }
}

struct InteractionState {
    TransformMode activeTool = TransformMode::Move;
    InteractionPhase phase = InteractionPhase::Idle;
    AxisConstraint axisConstraint = AxisConstraint::None;

    // Dữ liệu giao dịch biến đổi (Transaction boundary - phục vụ Cancel / Esc và Undo/Redo sau này)
    scene::ObjectId activeObjectId = scene::kInvalidObjectId;
    scene::Transform initialTransform;
    scene::Transform lastValidTransform;
    float dragPlaneY = 0.0f;
    glm::vec3 grabOffset{0.0f, 0.0f, 0.0f};
    glm::vec2 startMousePos{0.0f, 0.0f};
    glm::vec2 totalMouseDelta{0.0f, 0.0f};
    bool hasValidDragAnchor = false;

    // Trạng thái snapping, Move Gizmo & phản hồi giao diện
    bool gridSnapEnabled = false;
    float angleSnapDegrees = 15.0f;
    bool gizmoEnabled = true;
    AxisConstraint hoveredGizmoAxis = AxisConstraint::None;
    bool startedFromGizmo = false;
    std::optional<glm::vec3> hoverWorldPos;
    std::string warningMessage;

    bool isIdle() const { return phase == InteractionPhase::Idle; }
    bool isCameraInteracting() const
    {
        return phase == InteractionPhase::Orbiting || phase == InteractionPhase::Panning;
    }
    bool isTransformInteracting() const
    {
        return phase == InteractionPhase::Moving ||
               phase == InteractionPhase::Rotating ||
               phase == InteractionPhase::Scaling;
    }

    void resetToIdle()
    {
        phase = InteractionPhase::Idle;
        axisConstraint = AxisConstraint::None;
        activeObjectId = scene::kInvalidObjectId;
        grabOffset = glm::vec3(0.0f);
        startMousePos = glm::vec2(0.0f);
        totalMouseDelta = glm::vec2(0.0f);
        hasValidDragAnchor = false;
        startedFromGizmo = false;
    }

    // Kiểm tra tính hợp lệ khi scene thay đổi đột ngột (xóa object / chuyển preset / khóa object)
    void abortIfInvalid(const scene::Scene& scene)
    {
        if (!isTransformInteracting()) {
            return;
        }
        const auto* obj = scene.findObject(activeObjectId);
        if (obj == nullptr || obj->locked) {
            resetToIdle();
        }
    }
};

} // namespace roomcraft::systems
