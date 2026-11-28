#include "roomcraft/editor/ViewportInteraction.hpp"

#include "roomcraft/math/Intersection.hpp"
#include "roomcraft/math/Plane.hpp"
#include "roomcraft/systems/TransformController.hpp"
#include "roomcraft/systems/TransformGizmo.hpp"

#include <glm/vec2.hpp>

#include <cmath>

namespace roomcraft::editor {

ViewportInteractionOutcome ViewportInteraction::processInput(const core::InputManager& input,
                                                             const ViewportInputContext& ctx,
                                                             scene::Scene& scene,
                                                             systems::SelectionState& selection,
                                                             systems::CameraController& cameraController,
                                                             graphics::Camera& camera,
                                                             systems::InteractionState& state,
                                                             const systems::PickingSystem::AssetDataLookupFn& assetLookup,
                                                             const systems::PickOptions& pickOptions)
{
    ViewportInteractionOutcome out;
    selection.reconcileWithScene(scene);
    state.abortIfInvalid(scene);

    const glm::vec2 mousePos = input.getMousePosition();
    const glm::vec2 mouseDelta = input.getMouseDelta();

    // Điều kiện hợp lệ để BẮT ĐẦU một tương tác mới trong ảnh Viewport 3D:
    // KHÔNG chỉ dựa vào ImGui::GetIO().WantCaptureMouse (vì viewport nằm trong cửa sổ ImGui),
    // mà kết hợp trực tiếp image rectangle, hover state, active UI widgets và popup.
    const bool canStartViewportInput = ctx.viewportImageHovered &&
                                       ctx.viewportRect.contains(mousePos.x, mousePos.y) &&
                                       !ctx.uiPopupOpen &&
                                       !ctx.anyUiWidgetActive &&
                                       !ctx.uiWantsTextInput;

    // Cập nhật tọa độ con trỏ chiếu lên mặt sàn Y = 0 & trạng thái hover của Move Gizmo 3 trục
    state.hoverWorldPos.reset();
    state.hoveredGizmoAxis = systems::AxisConstraint::None;
    if (ctx.viewportRect.contains(mousePos.x, mousePos.y) && !ctx.uiPopupOpen) {
        math::Ray hoverRay;
        if (math::Ray::screenToWorldRay(mousePos.x, mousePos.y, ctx.viewportRect,
                                        camera.getViewMatrix(), camera.getProjectionMatrix(), hoverRay, true)) {
            math::RayPlaneHit groundHit;
            if (math::intersectRayPlane(hoverRay, math::Plane::horizontalAtY(0.0f), groundHit, true)) {
                state.hoverWorldPos = groundHit.point;
            }
            if (state.gizmoEnabled &&
                state.activeTool == systems::TransformMode::Move &&
                state.isIdle() &&
                selection.hasSelection()) {
                if (const auto* selObj = scene.findObject(selection.getSelectedObjectId())) {
                    const float gizmoLen = systems::TransformGizmo::computeHandleLength(selObj->transform.position, camera);
                    state.hoveredGizmoAxis = systems::TransformGizmo::pickAxis(hoverRay, selObj->transform.position, gizmoLen);
                }
            }
        }
    }

    // ==================== 1. PHÍM TẮT BÀN PHÍM (KHI KHÔNG GÕ TEXT / POPUP) ====================
    if (!ctx.uiWantsTextInput && !ctx.uiPopupOpen) {
        // Phím Esc (ASCII 27): ưu tiên hủy thao tác đang diễn ra và khôi phục Transform ban đầu
        if (input.isKeyPressed(27)) {
            if (state.isTransformInteracting()) {
                systems::TransformController::cancelInteraction(scene, state);
                out.consumedEscape = true;
                out.statusMessage = "Cancelled active transform (restored initial transform).";
                return out;
            }
            if (state.isCameraInteracting()) {
                state.resetToIdle();
                out.consumedEscape = true;
                out.statusMessage = "Cancelled camera interaction.";
                return out;
            }
            if (state.axisConstraint != systems::AxisConstraint::None) {
                state.axisConstraint = systems::AxisConstraint::None;
                out.consumedEscape = true;
                out.statusMessage = "Cleared axis constraint.";
                return out;
            }
        }

        // Phím chuyển chế độ thao tác: Q=Select, G=Move, R=Rotate, S=Scale
        if (state.isIdle()) {
            if (input.isKeyPressed('q')) {
                state.activeTool = systems::TransformMode::Select;
                state.axisConstraint = systems::AxisConstraint::None;
                out.statusMessage = "Mode: Select";
            } else if (input.isKeyPressed('g')) {
                state.activeTool = systems::TransformMode::Move;
                state.axisConstraint = systems::AxisConstraint::None;
                out.statusMessage = "Mode: Move (Drag left mouse on object)";
            } else if (input.isKeyPressed('r')) {
                state.activeTool = systems::TransformMode::Rotate;
                state.axisConstraint = systems::AxisConstraint::None;
                out.statusMessage = "Mode: Rotate (Drag left mouse horizontally)";
            } else if (input.isKeyPressed('s')) {
                state.activeTool = systems::TransformMode::Scale;
                state.axisConstraint = systems::AxisConstraint::None;
                out.statusMessage = "Mode: Scale (Drag left mouse horizontally)";
            }
        }

        // Phím giới hạn trục X / Y / Z trong chế độ biến đổi
        if (state.activeTool != systems::TransformMode::Select || state.isTransformInteracting()) {
            if (input.isKeyPressed('x')) {
                state.axisConstraint = (state.axisConstraint == systems::AxisConstraint::X)
                                           ? systems::AxisConstraint::None
                                           : systems::AxisConstraint::X;
                out.statusMessage = std::string("Axis constraint: ") + systems::axisConstraintToString(state.axisConstraint);
            } else if (input.isKeyPressed('y')) {
                state.axisConstraint = (state.axisConstraint == systems::AxisConstraint::Y)
                                           ? systems::AxisConstraint::None
                                           : systems::AxisConstraint::Y;
                out.statusMessage = std::string("Axis constraint: ") + systems::axisConstraintToString(state.axisConstraint);
            } else if (input.isKeyPressed('z')) {
                state.axisConstraint = (state.axisConstraint == systems::AxisConstraint::Z)
                                           ? systems::AxisConstraint::None
                                           : systems::AxisConstraint::Z;
                out.statusMessage = std::string("Axis constraint: ") + systems::axisConstraintToString(state.axisConstraint);
            }
        }

        // Phím tắt Camera Presets & Focus (chỉ khi không đang kéo biến đổi vật thể)
        if (!state.isTransformInteracting()) {
            if (input.isKeyPressed('1')) {
                cameraController.setTopViewPreset(scene.getRoom().getExteriorBounds(), camera);
                out.statusMessage = "Camera: Orthographic Top View (1)";
            } else if (input.isKeyPressed('2')) {
                cameraController.setPerspectivePreset(scene.getRoom().getExteriorBounds(), camera);
                out.statusMessage = "Camera: Perspective 3D View (2)";
            } else if (input.isKeyPressed('3')) {
                cameraController.setIsometricPreset(scene.getRoom().getExteriorBounds(), camera);
                out.statusMessage = "Camera: Isometric View (3)";
            } else if (input.isSpecialKeyPressed(core::SpecialKey::Home)) {
                cameraController.fitBoundingBox(scene.getRoom().getExteriorBounds(), camera, 1.22f);
                out.statusMessage = "Camera: Fit Room (Home)";
            } else if (input.isKeyPressed('f')) {
                if (selection.hasSelection()) {
                    const auto* obj = scene.findObject(selection.getSelectedObjectId());
                    const assets::AssetData* asset = (obj != nullptr && assetLookup) ? assetLookup(obj->assetId) : nullptr;
                    if (obj != nullptr && asset != nullptr && asset->localBounds.isValid()) {
                        cameraController.fitBoundingBox(obj->getWorldBounds(asset->localBounds), camera, 1.35f);
                        out.statusMessage = "Focused on '" + obj->name + "' (F)";
                    }
                }
            }
        }

        // Delete (ASCII 127): xóa đối tượng đang chọn (tôn trọng khóa locked & hủy tương tác an toàn)
        if (input.isKeyPressed(127) && selection.hasSelection()) {
            const auto targetId = selection.getSelectedObjectId();
            const auto* obj = scene.findObject(targetId);
            if (obj != nullptr && obj->locked) {
                state.warningMessage = "Cannot delete locked object '" + obj->name + "'.";
                out.errorMessage = state.warningMessage;
            } else if (obj != nullptr) {
                if (state.activeObjectId == targetId) {
                    state.resetToIdle();
                }
                const std::string removedName = obj->name;
                if (scene.removeObject(targetId)) {
                    selection.clearSelection();
                    out.sceneModified = true;
                    out.statusMessage = "Deleted '" + removedName + "' (Delete).";
                }
            }
        }

        // Ctrl + D (ASCII 0x04 trên FreeGLUT Windows hoặc 'd' kèm Ctrl): nhân bản đối tượng đang chọn
        const bool ctrlDPressed = input.isKeyPressed(0x04) || (input.isCtrlDown() && input.isKeyPressed('d'));
        if (ctrlDPressed && selection.hasSelection() && !state.isTransformInteracting()) {
            std::string err;
            const auto newId = scene.duplicateObject(selection.getSelectedObjectId(), glm::vec3(0.45f, 0.0f, 0.45f), &err);
            if (scene::isValidObjectId(newId)) {
                selection.selectObject(newId);
                out.sceneModified = true;
                out.statusMessage = "Duplicated object -> ID #" + std::to_string(newId) + " (Ctrl+D).";
            } else {
                out.errorMessage = err;
            }
        }
    }

    // ==================== 2. CON LĂN CHUỘT (ZOOM CAMERA) ====================
    if (canStartViewportInput && std::fabs(input.getWheelDelta()) > 1e-5f) {
        cameraController.zoom(input.getWheelDelta(), camera);
    }

    // ==================== 3. TIẾP TỤC TƯƠNG TÁC ĐANG HOẠT ĐỘNG ====================
    if (state.phase == systems::InteractionPhase::Orbiting) {
        if (input.isMouseHeld(core::MouseButton::Right)) {
            if (std::fabs(mouseDelta.x) > 1e-4f || std::fabs(mouseDelta.y) > 1e-4f) {
                cameraController.orbit(mouseDelta.x, mouseDelta.y, camera);
            }
        } else {
            state.resetToIdle();
        }
        return out;
    }

    if (state.phase == systems::InteractionPhase::Panning) {
        if (input.isMouseHeld(core::MouseButton::Middle)) {
            if (std::fabs(mouseDelta.x) > 1e-4f || std::fabs(mouseDelta.y) > 1e-4f) {
                cameraController.pan(mouseDelta.x, mouseDelta.y,
                                     ctx.viewportRect.getWidth(), ctx.viewportRect.getHeight(), camera);
            }
        } else {
            state.resetToIdle();
        }
        return out;
    }

    if (state.phase == systems::InteractionPhase::Moving) {
        if (input.isMouseHeld(core::MouseButton::Left)) {
            math::Ray moveRay;
            if (math::Ray::screenToWorldRay(mousePos.x, mousePos.y, ctx.viewportRect,
                                            camera.getViewMatrix(), camera.getProjectionMatrix(), moveRay, false)) {
                std::string err;
                if (systems::TransformController::updateMove(scene, moveRay, mousePos, state, &err)) {
                    out.sceneModified = true;
                }
            }
        } else {
            systems::TransformController::confirmInteraction(state);
            out.statusMessage = "Move confirmed.";
        }
        return out;
    }

    if (state.phase == systems::InteractionPhase::Rotating) {
        if (input.isMouseHeld(core::MouseButton::Left)) {
            std::string err;
            if (systems::TransformController::updateRotate(scene, mousePos, state, &err)) {
                out.sceneModified = true;
            }
        } else {
            systems::TransformController::confirmInteraction(state);
            out.statusMessage = "Rotation confirmed.";
        }
        return out;
    }

    if (state.phase == systems::InteractionPhase::Scaling) {
        if (input.isMouseHeld(core::MouseButton::Left)) {
            std::string err;
            if (systems::TransformController::updateScale(scene, mousePos, state, &err)) {
                out.sceneModified = true;
            }
        } else {
            systems::TransformController::confirmInteraction(state);
            out.statusMessage = "Scale confirmed.";
        }
        return out;
    }

    // ==================== 4. KHỞI ĐỘNG TƯƠNG TÁC MỚI KHI ĐANG IDLE ====================
    if (!canStartViewportInput) {
        return out;
    }

    // Chuột phải -> Orbit Camera
    if (input.isMousePressed(core::MouseButton::Right)) {
        state.phase = systems::InteractionPhase::Orbiting;
        state.startMousePos = mousePos;
        return out;
    }

    // Chuột giữa -> Pan Camera
    if (input.isMousePressed(core::MouseButton::Middle)) {
        state.phase = systems::InteractionPhase::Panning;
        state.startMousePos = mousePos;
        return out;
    }

    // Chuột trái -> Ưu tiên Move Gizmo 3 trục (nếu đang bật) trước Picking vật thể phía sau
    if (input.isMousePressed(core::MouseButton::Left)) {
        math::Ray pickRay;
        if (!math::Ray::screenToWorldRay(mousePos.x, mousePos.y, ctx.viewportRect,
                                         camera.getViewMatrix(), camera.getProjectionMatrix(), pickRay, true)) {
            return out;
        }

        // 4a. Kiểm tra hit-testing Move Gizmo 3 trục trước (không bao giờ chọn nhầm vật thể phía sau handle)
        if (state.gizmoEnabled &&
            state.activeTool == systems::TransformMode::Move &&
            selection.hasSelection()) {
            if (const auto* selObj = scene.findObject(selection.getSelectedObjectId())) {
                const float gizmoLen = systems::TransformGizmo::computeHandleLength(selObj->transform.position, camera);
                const auto hitAxis = systems::TransformGizmo::pickAxis(pickRay, selObj->transform.position, gizmoLen);
                if (hitAxis != systems::AxisConstraint::None) {
                    state.axisConstraint = hitAxis;
                    state.hoveredGizmoAxis = hitAxis;
                    std::string err;
                    if (systems::TransformController::beginMove(scene, selObj->id, pickRay, mousePos, state, &err)) {
                        state.startedFromGizmo = true;
                        out.statusMessage = std::string("Move Gizmo: ") + systems::axisConstraintToString(hitAxis);
                    } else {
                        state.axisConstraint = systems::AxisConstraint::None;
                        out.errorMessage = err;
                    }
                    return out;
                }
            }
        }

        const auto pick = systems::PickingSystem::pickObject(pickRay, scene, assetLookup, pickOptions);

        if (pick.hit) {
            selection.selectObject(pick.objectId);
            const auto* pickedObj = scene.findObject(pick.objectId);
            state.warningMessage.clear();

            if (pickedObj != nullptr) {
                out.statusMessage = "Selected '" + pickedObj->name + "' [#" + std::to_string(pickedObj->id) + "]";
            }

            if (state.activeTool == systems::TransformMode::Move) {
                std::string err;
                if (!systems::TransformController::beginMove(scene, pick.objectId, pickRay, mousePos, state, &err)) {
                    out.errorMessage = err;
                }
            } else if (state.activeTool == systems::TransformMode::Rotate) {
                std::string err;
                if (!systems::TransformController::beginRotate(scene, pick.objectId, mousePos, state, &err)) {
                    out.errorMessage = err;
                }
            } else if (state.activeTool == systems::TransformMode::Scale) {
                std::string err;
                if (!systems::TransformController::beginScale(scene, pick.objectId, mousePos, state, &err)) {
                    out.errorMessage = err;
                }
            }
        } else {
            // Khi đang ở Rotate/Scale và đã chọn sẵn vật thể -> cho phép kéo xoay/scale ngay cả khi kéo vùng trống;
            // ở Select/Move -> nhấn vào khoảng trống sẽ xóa chọn (clear selection)
            if ((state.activeTool == systems::TransformMode::Rotate ||
                 state.activeTool == systems::TransformMode::Scale) &&
                selection.hasSelection()) {
                std::string err;
                if (state.activeTool == systems::TransformMode::Rotate) {
                    if (!systems::TransformController::beginRotate(scene, selection.getSelectedObjectId(), mousePos, state, &err)) {
                        out.errorMessage = err;
                    }
                } else {
                    if (!systems::TransformController::beginScale(scene, selection.getSelectedObjectId(), mousePos, state, &err)) {
                        out.errorMessage = err;
                    }
                }
            } else {
                selection.clearSelection();
                state.warningMessage.clear();
                out.statusMessage = pick.occludedByWall
                                        ? "Click blocked by visible wall (hide wall or orbit camera)."
                                        : "Selection cleared.";
            }
        }
    }

    return out;
}

} // namespace roomcraft::editor
