#include "roomcraft/assets/AssetData.hpp"
#include "roomcraft/assets/FurnitureCatalog.hpp"
#include "roomcraft/core/InputManager.hpp"
#include "roomcraft/editor/EditorLayout.hpp"
#include "roomcraft/editor/NotificationSystem.hpp"
#include "roomcraft/editor/ViewportInteraction.hpp"
#include "roomcraft/geometry/FurnitureGenerator.hpp"
#include "roomcraft/graphics/Camera.hpp"
#include "roomcraft/math/Ray.hpp"
#include "roomcraft/scene/Scene.hpp"
#include "roomcraft/systems/CameraController.hpp"
#include "roomcraft/systems/TransformGizmo.hpp"

#include <cmath>
#include <iostream>
#include <unordered_map>

namespace {

bool approx(float a, float b, float eps = 1e-2f)
{
    return std::fabs(a - b) <= eps;
}

bool check(bool cond, const char* label, int& failures)
{
    std::cout << "  [M6-UI] " << label << ": " << (cond ? "PASS" : "FAIL") << std::endl;
    if (!cond) ++failures;
    return cond;
}

bool verifyNoOverlapLayout(const roomcraft::editor::ComputedWorkspaceLayout& l, float winW, float winH)
{
    if (!l.centerViewport.isValid() || l.centerViewport.width < 240.0f) return false;
    if (l.leftPanelVisible && (l.leftPanel.x + l.leftPanel.width > l.centerViewport.x + 0.5f)) return false;
    if (l.rightPanelVisible && (l.centerViewport.x + l.centerViewport.width > l.rightTopPanel.x + 0.5f)) return false;
    if (l.rightPanelVisible && (l.rightTopPanel.y + l.rightTopPanel.height > l.rightBottomPanel.y + 0.5f)) return false;
    if (l.statusBar.y + l.statusBar.height > winH + 1.0f) return false;
    if (l.rightPanelVisible && (l.rightTopPanel.x + l.rightTopPanel.width > winW + 1.0f)) return false;
    return true;
}

} // namespace

int main()
{
    std::cout << "================ RoomCraft 3D - M6 Editor UI & Gizmo Unit Tests (CPU) ================" << std::endl;
    int failures = 0;

    // 1. TC-M6-08/09/10: Layout validity across 1280x720, 1366x768, 1600x900, 1920x1080
    {
        roomcraft::editor::EditorLayout layout;
        const float sizes[4][2] = {{1280, 720}, {1366, 768}, {1600, 900}, {1920, 1080}};
        bool allOk = true;
        for (const auto& sz : sizes) {
            const auto comp = layout.compute(sz[0], sz[1], 24.0f);
            const bool centerLargest = comp.centerViewport.width > comp.leftPanel.width &&
                                       comp.centerViewport.width > comp.rightTopPanel.width;
            allOk &= verifyNoOverlapLayout(comp, sz[0], sz[1]) && centerLargest;
        }
        check(allOk, "TC-M6-08..10 Layout non-overlapping & viewport largest at 1280x720, 1366x768, 1600x900, 1920x1080", failures);
    }

    // 2. TC-M6-11/12: DPI scaling (100%, 125%, 150%) & interactive panel resizing preserve usable viewport
    {
        roomcraft::editor::EditorLayout layout;
        bool dpiOk = true;
        for (float s : {1.0f, 1.25f, 1.50f}) {
            layout.setUiScale(s);
            const auto comp = layout.compute(1280.0f, 720.0f, 24.0f * s);
            dpiOk &= verifyNoOverlapLayout(comp, 1280.0f, 720.0f) && (comp.centerViewport.width >= 380.0f);
        }
        layout.setUiScale(1.0f);
        layout.setLeftPanelWidth(340.0f);
        layout.setRightPanelWidth(350.0f);
        layout.setHierarchyHeightRatio(0.55f);
        const auto resized = layout.compute(1600.0f, 900.0f, 24.0f);
        const bool resizeOk = approx(resized.leftPanel.width, 340.0f, 1.0f) &&
                              approx(resized.rightTopPanel.width, 350.0f, 1.0f) &&
                              verifyNoOverlapLayout(resized, 1600.0f, 900.0f);
        check(dpiOk && resizeOk, "TC-M6-11/12 DPI scale (100%/125%/150%) & splitter resizing keep viewport usable", failures);
    }

    // 3. TC-M6-13/14/15: Catalog completeness (14 assets), search & category filtering
    {
        roomcraft::assets::FurnitureCatalog cat;
        const bool has14 = (cat.getAllEntries().size() == 14);
        const auto sofas = cat.filter(std::nullopt, std::nullopt, "sofa");
        const auto lighting = cat.filter(std::nullopt, roomcraft::assets::AssetCategory::Lighting, "");
        const auto procedural = cat.filter(roomcraft::assets::AssetSourceType::Procedural, std::nullopt, "");
        const auto blender = cat.filter(roomcraft::assets::AssetSourceType::Imported, std::nullopt, "");
        check(has14 && sofas.size() == 1 && lighting.size() == 2 && procedural.size() == 6 && blender.size() == 8,
              "TC-M6-13..15 FurnitureCatalog has 14 assets (6 C++ + 8 Blender) with accurate search/category filter", failures);
    }

    // 4. TC-M6-25: NotificationSystem push, cap & time expiration
    {
        roomcraft::editor::NotificationSystem notif;
        notif.push(roomcraft::editor::NotificationLevel::Info, "Info 1", 1.0f);
        notif.push(roomcraft::editor::NotificationLevel::Success, "Success 2", 2.0f);
        notif.push(roomcraft::editor::NotificationLevel::Warning, "Warn 3", 3.0f);
        notif.push(roomcraft::editor::NotificationLevel::Error, "Err 4", 4.0f);
        notif.push(roomcraft::editor::NotificationLevel::Info, "Info 5", 5.0f);
        const bool capped = (notif.getActive().size() == roomcraft::editor::NotificationSystem::kMaxVisible);
        notif.update(2.5f); // Info 1 already evicted; Success 2 (2.0s) expires -> 3 remain
        const bool expired = (notif.getActive().size() == 3);
        notif.clear();
        check(capped && expired && notif.empty(),
              "TC-M6-25 NotificationSystem queue cap, expiration & clear lifecycle", failures);
    }

    // 5. TC-M6-27/28 & TC-M6-GIZMO: 3-Axis Move Gizmo hit-testing, priority over object behind, axis-constrained drag,
    //    Esc Cancel, locked object protection, and picking accuracy after panel resize + 1.5x DPI scaling
    {
        roomcraft::assets::FurnitureCatalog catalog;
        std::unordered_map<std::string, roomcraft::assets::AssetData> cpuAssets;
        roomcraft::geometry::FurnitureModel deskModel;
        roomcraft::geometry::FurnitureGenerator::createDesk(roomcraft::geometry::DeskParams{}, deskModel);
        cpuAssets["procedural-desk"] = roomcraft::assets::AssetData::fromFurnitureModel(
            "procedural-desk", roomcraft::assets::AssetCategory::Tables, deskModel);
        const auto lookup = [&cpuAssets](const std::string& id) -> const roomcraft::assets::AssetData* {
            const auto it = cpuAssets.find(id);
            return (it != cpuAssets.end()) ? &it->second : nullptr;
        };

        roomcraft::scene::Scene scene;
        roomcraft::systems::SelectionState selection;
        roomcraft::systems::CameraController camCtrl;
        roomcraft::systems::InteractionState state;
        roomcraft::graphics::Camera cam;
        cam.setAspectRatio(800.0f / 600.0f);
        camCtrl.setPerspectivePreset(scene.getRoom().getExteriorBounds(), cam);

        const auto deskId = scene.addObject("procedural-desk", "Desk",
                                            roomcraft::scene::Transform({0.0f, 0.0f, 0.0f}), catalog);
        const auto behindId = scene.addObject("procedural-desk", "Desk Behind X",
                                              roomcraft::scene::Transform({1.2f, 0.0f, -2.0f}), catalog);
        selection.selectObject(deskId);

        // Gizmo handle length scales cleanly with distance
        const float len = roomcraft::systems::TransformGizmo::computeHandleLength({0, 0, 0}, cam);
        roomcraft::math::Ray rx, ry, rz;
        roomcraft::math::Ray::create({len * 0.6f, 0.0f, 4.0f}, {0.0f, 0.0f, -1.0f}, rx);
        roomcraft::math::Ray::create({0.0f, len * 0.6f, 4.0f}, {0.0f, 0.0f, -1.0f}, ry);
        roomcraft::math::Ray::create({4.0f, 0.0f, len * 0.6f}, {-1.0f, 0.0f, 0.0f}, rz);

        const bool axisHitsOk =
            roomcraft::systems::TransformGizmo::pickAxis(rx, {0, 0, 0}, len) == roomcraft::systems::AxisConstraint::X &&
            roomcraft::systems::TransformGizmo::pickAxis(ry, {0, 0, 0}, len) == roomcraft::systems::AxisConstraint::Y &&
            roomcraft::systems::TransformGizmo::pickAxis(rz, {0, 0, 0}, len) == roomcraft::systems::AxisConstraint::Z;

        // Chiếu đầu mũi tên X của gizmo vào viewport bị dịch sau resize + 1.5x DPI -> bấm chuột trái phải nắm trục X
        // và KHÔNG chọn nhầm `behindId` phía sau!
        roomcraft::editor::ViewportInputContext ctx;
        ctx.viewportRect = {340.0f, 64.0f, 1140.0f, 664.0f, 1200.0f, 900.0f}; // 800x600 display @ 1.5x DPI
        ctx.viewportImageHovered = true;

        const glm::vec3 xHandleWorld(len * 0.65f, 0.0f, 0.0f);
        const glm::vec4 clip = cam.getProjectionMatrix() * cam.getViewMatrix() * glm::vec4(xHandleWorld, 1.0f);
        const glm::vec3 ndc = glm::vec3(clip) / clip.w;
        const float sx = ctx.viewportRect.minX + (ndc.x * 0.5f + 0.5f) * ctx.viewportRect.getWidth();
        const float sy = ctx.viewportRect.minY + (0.5f - ndc.y * 0.5f) * ctx.viewportRect.getHeight();

        roomcraft::core::InputManager input;
        state.activeTool = roomcraft::systems::TransformMode::Move;
        state.gizmoEnabled = true;

        input.onMouseButton(roomcraft::core::MouseButton::Left, true, sx, sy);
        roomcraft::editor::ViewportInteraction::processInput(input, ctx, scene, selection, camCtrl, cam, state, lookup);
        const bool grabbedAxisX = (state.phase == roomcraft::systems::InteractionPhase::Moving) &&
                                  (state.axisConstraint == roomcraft::systems::AxisConstraint::X) &&
                                  state.startedFromGizmo &&
                                  selection.isSelected(deskId);
        input.endFrame();

        // Kéo chéo chuột -> chỉ di chuyển theo X, giữ Z = 0 và Y = 0!
        input.onMouseMove(sx + 90.0f, sy + 50.0f);
        roomcraft::editor::ViewportInteraction::processInput(input, ctx, scene, selection, camCtrl, cam, state, lookup);
        const auto posAfterDrag = scene.findObject(deskId)->transform.position;
        const bool movedOnlyX = (std::fabs(posAfterDrag.x) > 0.1f) &&
                                approx(posAfterDrag.y, 0.0f) &&
                                approx(posAfterDrag.z, 0.0f);
        input.endFrame();

        // Nhấn Esc hủy thao tác -> khôi phục về (0, 0, 0)
        input.onKeyDown(27, false, false, false);
        const auto cancelOut = roomcraft::editor::ViewportInteraction::processInput(
            input, ctx, scene, selection, camCtrl, cam, state, lookup);
        const auto posAfterCancel = scene.findObject(deskId)->transform.position;
        const bool cancelRestored = cancelOut.consumedEscape && state.isIdle() &&
                                    approx(posAfterCancel.x, 0.0f) && approx(posAfterCancel.z, 0.0f);
        input.endFrame();
        input.onMouseButton(roomcraft::core::MouseButton::Left, false, sx + 90.0f, sy + 50.0f);
        input.endFrame();

        // Kiểm tra locked object không kéo được qua gizmo
        scene.findObject(deskId)->locked = true;
        input.onMouseButton(roomcraft::core::MouseButton::Left, true, sx, sy);
        const auto lockedOut = roomcraft::editor::ViewportInteraction::processInput(
            input, ctx, scene, selection, camCtrl, cam, state, lookup);
        const bool lockedBlocked = state.isIdle() && !lockedOut.errorMessage.empty() && selection.isSelected(deskId);

        (void)behindId;
        check(axisHitsOk && grabbedAxisX && movedOnlyX && cancelRestored && lockedBlocked,
              "TC-M6-27/28 & TC-M6-GIZMO 3-Axis Move Gizmo hit, X-constrained drag after resize/DPI, Esc cancel & locked block",
              failures);
    }

    std::cout << "======================================================================================" << std::endl;
    if (failures == 0) {
        std::cout << "[M6-UNIT-TESTS] ALL PASSED (0 failures)." << std::endl;
        return 0;
    }
    std::cerr << "[M6-UNIT-TESTS] FAILED with " << failures << " failure(s)." << std::endl;
    return 1;
}
