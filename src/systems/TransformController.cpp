#include "roomcraft/systems/TransformController.hpp"

#include "roomcraft/math/Intersection.hpp"
#include "roomcraft/math/Plane.hpp"
#include "roomcraft/systems/PlacementSystem.hpp"

#include <algorithm>
#include <cmath>

namespace roomcraft::systems {

namespace {

bool validateTargetObjectForEdit(const scene::Scene& scene,
                                 scene::ObjectId objectId,
                                 InteractionState& state,
                                 const scene::SceneObject*& outObj,
                                 std::string* outError)
{
    if (!state.isIdle()) {
        if (outError != nullptr) *outError = "Another interaction is currently active.";
        return false;
    }
    const auto* obj = scene.findObject(objectId);
    if (obj == nullptr) {
        if (outError != nullptr) *outError = "Selected object no longer exists.";
        return false;
    }
    if (obj->locked) {
        state.warningMessage = "Object '" + obj->name + "' is locked and cannot be transformed.";
        if (outError != nullptr) *outError = state.warningMessage;
        return false;
    }
    outObj = obj;
    return true;
}

} // namespace

bool TransformController::beginMove(scene::Scene& scene,
                                    scene::ObjectId objectId,
                                    const math::Ray& startRay,
                                    const glm::vec2& startMousePos,
                                    InteractionState& state,
                                    std::string* outError)
{
    const scene::SceneObject* obj = nullptr;
    if (!validateTargetObjectForEdit(scene, objectId, state, obj, outError)) {
        return false;
    }

    const float planeY = obj->transform.position.y;
    const math::Plane dragPlane = math::Plane::horizontalAtY(planeY);

    math::RayPlaneHit hit;
    const bool hitOk = math::intersectRayPlane(startRay, dragPlane, hit, true);

    state.phase = InteractionPhase::Moving;
    state.activeObjectId = obj->id;
    state.initialTransform = obj->transform;
    state.lastValidTransform = obj->transform;
    state.dragPlaneY = planeY;
    state.startMousePos = startMousePos;
    state.totalMouseDelta = glm::vec2(0.0f);
    state.warningMessage.clear();

    if (hitOk) {
        // Lưu grab offset để đối tượng không nhảy về tâm con trỏ khi mới nhấn giữ chuột
        state.grabOffset = obj->transform.position - hit.point;
        state.grabOffset.y = 0.0f;
        state.hasValidDragAnchor = true;
    } else {
        state.grabOffset = glm::vec3(0.0f);
        state.hasValidDragAnchor = false;
    }
    return true;
}

bool TransformController::updateMove(scene::Scene& scene,
                                     const math::Ray& currentRay,
                                     const glm::vec2& currentMousePos,
                                     InteractionState& state,
                                     std::string* outError)
{
    if (state.phase != InteractionPhase::Moving) {
        return false;
    }

    auto* obj = scene.findObject(state.activeObjectId);
    if (obj == nullptr || obj->locked) {
        state.resetToIdle();
        return false;
    }

    state.totalMouseDelta = currentMousePos - state.startMousePos;
    scene::Transform candidate = state.initialTransform;

    if (state.axisConstraint == AxisConstraint::Y) {
        // Kéo theo phương thẳng đứng Y bằng dịch chuyển chuột dọc (kéo lên -> tăng Y)
        constexpr float kVerticalMetersPerPixel = 0.01f;
        float nextY = state.initialTransform.position.y - state.totalMouseDelta.y * kVerticalMetersPerPixel;
        if (state.gridSnapEnabled) {
            const float step = std::max(0.05f, scene.getRoom().getSettings().gridSpacing * 0.5f);
            nextY = std::round(nextY / step) * step;
        }
        candidate.position.y = std::clamp(nextY, 0.0f, scene.getRoom().getSettings().height);
    } else {
        const math::Plane dragPlane = math::Plane::horizontalAtY(state.dragPlaneY);
        math::RayPlaneHit hit;
        if (!math::intersectRayPlane(currentRay, dragPlane, hit, true)) {
            // Tia gần song song mặt phẳng kéo: giữ nguyên transform hợp lệ gần nhất, không nhảy đột ngột
            return false;
        }

        if (!state.hasValidDragAnchor) {
            state.grabOffset = state.initialTransform.position - hit.point;
            state.grabOffset.y = 0.0f;
            state.hasValidDragAnchor = true;
        }

        glm::vec3 targetPos = hit.point + state.grabOffset;
        // Luôn bảo toàn cao độ Y ban đầu (Computer Setup / Desk Lamp / Vase trên bàn không rơi xuống sàn)
        targetPos.y = state.initialTransform.position.y;

        if (state.axisConstraint == AxisConstraint::X) {
            targetPos.z = state.initialTransform.position.z;
        } else if (state.axisConstraint == AxisConstraint::Z) {
            targetPos.x = state.initialTransform.position.x;
        }

        // Tính Grid Snapping trực tiếp từ targetPos chưa snap để không sinh sai số tích lũy qua từng frame
        if (state.gridSnapEnabled) {
            targetPos = PlacementSystem::snapToGrid(targetPos, scene.getRoom().getSettings().gridSpacing, false);
            targetPos.y = state.initialTransform.position.y;
            if (state.axisConstraint == AxisConstraint::X) {
                targetPos.z = state.initialTransform.position.z;
            } else if (state.axisConstraint == AxisConstraint::Z) {
                targetPos.x = state.initialTransform.position.x;
            }
        }

        // Giới hạn biên an toàn tránh giá trị cực lớn khi tia quét sát đường chân trời
        targetPos.x = std::clamp(targetPos.x, -30.0f, 30.0f);
        targetPos.z = std::clamp(targetPos.z, -30.0f, 30.0f);
        candidate.position = targetPos;
    }

    if (!scene.setObjectTransform(state.activeObjectId, candidate, outError)) {
        return false;
    }

    state.lastValidTransform = candidate;
    return true;
}

bool TransformController::beginRotate(scene::Scene& scene,
                                      scene::ObjectId objectId,
                                      const glm::vec2& startMousePos,
                                      InteractionState& state,
                                      std::string* outError)
{
    const scene::SceneObject* obj = nullptr;
    if (!validateTargetObjectForEdit(scene, objectId, state, obj, outError)) {
        return false;
    }

    state.phase = InteractionPhase::Rotating;
    state.activeObjectId = obj->id;
    state.initialTransform = obj->transform;
    state.lastValidTransform = obj->transform;
    state.startMousePos = startMousePos;
    state.totalMouseDelta = glm::vec2(0.0f);
    state.warningMessage.clear();
    return true;
}

bool TransformController::updateRotate(scene::Scene& scene,
                                       const glm::vec2& currentMousePos,
                                       InteractionState& state,
                                       std::string* outError)
{
    if (state.phase != InteractionPhase::Rotating) {
        return false;
    }

    auto* obj = scene.findObject(state.activeObjectId);
    if (obj == nullptr || obj->locked) {
        state.resetToIdle();
        return false;
    }

    state.totalMouseDelta = currentMousePos - state.startMousePos;
    constexpr float kDegreesPerPixel = 0.60f;
    float deltaAngle = state.totalMouseDelta.x * kDegreesPerPixel;

    scene::Transform candidate = state.initialTransform;
    // Mặc định xoay quanh trục Y (quy ước M1: M = T * Ry * Rx * Rz * S)
    int axisIndex = 1;
    if (state.axisConstraint == AxisConstraint::X) axisIndex = 0;
    else if (state.axisConstraint == AxisConstraint::Z) axisIndex = 2;

    float targetAngle = state.initialTransform.rotationDegrees[axisIndex] + deltaAngle;
    if (state.gridSnapEnabled && state.angleSnapDegrees > 1e-3f) {
        targetAngle = std::round(targetAngle / state.angleSnapDegrees) * state.angleSnapDegrees;
    }

    // Chuẩn hóa góc về [-360, 360]
    targetAngle = std::fmod(targetAngle, 360.0f);
    candidate.rotationDegrees[axisIndex] = targetAngle;

    if (!scene.setObjectTransform(state.activeObjectId, candidate, outError)) {
        return false;
    }

    state.lastValidTransform = candidate;
    return true;
}

bool TransformController::beginScale(scene::Scene& scene,
                                     scene::ObjectId objectId,
                                     const glm::vec2& startMousePos,
                                     InteractionState& state,
                                     std::string* outError)
{
    const scene::SceneObject* obj = nullptr;
    if (!validateTargetObjectForEdit(scene, objectId, state, obj, outError)) {
        return false;
    }

    state.phase = InteractionPhase::Scaling;
    state.activeObjectId = obj->id;
    state.initialTransform = obj->transform;
    state.lastValidTransform = obj->transform;
    state.startMousePos = startMousePos;
    state.totalMouseDelta = glm::vec2(0.0f);
    state.warningMessage.clear();
    return true;
}

bool TransformController::updateScale(scene::Scene& scene,
                                      const glm::vec2& currentMousePos,
                                      InteractionState& state,
                                      std::string* outError)
{
    if (state.phase != InteractionPhase::Scaling) {
        return false;
    }

    auto* obj = scene.findObject(state.activeObjectId);
    if (obj == nullptr || obj->locked) {
        state.resetToIdle();
        return false;
    }

    state.totalMouseDelta = currentMousePos - state.startMousePos;
    // Dùng hàm mũ exp(delta * k) để hệ số tỷ lệ luôn dương, mượt và đối xứng khi kéo trái/phải
    constexpr float kScalePerPixel = 0.005f;
    float factor = std::exp(state.totalMouseDelta.x * kScalePerPixel);
    if (!std::isfinite(factor)) {
        return false;
    }
    factor = std::clamp(factor, kMinScale, kMaxScale);

    if (state.gridSnapEnabled) {
        factor = std::max(kMinScale, std::round(factor / 0.1f) * 0.1f);
    }

    scene::Transform candidate = state.initialTransform;
    auto clampComp = [](float v) { return std::clamp(v, kMinScale, kMaxScale); };

    if (state.axisConstraint == AxisConstraint::X) {
        candidate.scale.x = clampComp(state.initialTransform.scale.x * factor);
    } else if (state.axisConstraint == AxisConstraint::Y) {
        candidate.scale.y = clampComp(state.initialTransform.scale.y * factor);
    } else if (state.axisConstraint == AxisConstraint::Z) {
        candidate.scale.z = clampComp(state.initialTransform.scale.z * factor);
    } else {
        candidate.scale.x = clampComp(state.initialTransform.scale.x * factor);
        candidate.scale.y = clampComp(state.initialTransform.scale.y * factor);
        candidate.scale.z = clampComp(state.initialTransform.scale.z * factor);
    }

    if (!scene.setObjectTransform(state.activeObjectId, candidate, outError)) {
        return false;
    }

    state.lastValidTransform = candidate;
    return true;
}

void TransformController::confirmInteraction(InteractionState& state)
{
    state.resetToIdle();
}

bool TransformController::cancelInteraction(scene::Scene& scene, InteractionState& state)
{
    if (!state.isTransformInteracting()) {
        state.resetToIdle();
        return false;
    }

    const bool restored = scene.setObjectTransform(state.activeObjectId, state.initialTransform);
    state.resetToIdle();
    return restored;
}

} // namespace roomcraft::systems
