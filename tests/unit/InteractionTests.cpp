#include "roomcraft/assets/AssetData.hpp"
#include "roomcraft/assets/FurnitureCatalog.hpp"
#include "roomcraft/core/InputManager.hpp"
#include "roomcraft/editor/ViewportInteraction.hpp"
#include "roomcraft/geometry/FurnitureGenerator.hpp"
#include "roomcraft/graphics/Camera.hpp"
#include "roomcraft/scene/Scene.hpp"
#include "roomcraft/systems/CameraController.hpp"
#include "roomcraft/systems/InteractionState.hpp"
#include "roomcraft/systems/SelectionState.hpp"
#include "roomcraft/systems/TransformController.hpp"

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
    std::cout << "  [Interaction] " << label << ": " << (cond ? "PASS" : "FAIL") << std::endl;
    if (!cond) ++failures;
    return cond;
}

} // namespace

int runInteractionTests()
{
    int failures = 0;

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
    camCtrl.setTopViewPreset(scene.getRoom().getExteriorBounds(), cam);

    // Tạo vật thể đặt ở độ cao Y = 0.75m (giống máy tính trên mặt bàn)
    const auto objId = scene.addObject("procedural-desk", "Elevated Desk",
                                       roomcraft::scene::Transform({0.20f, 0.75f, -0.30f}), catalog);
    selection.selectObject(objId);

    // TC-M5-27: Dragging preserves Y = 0.75m & grab offset (không nhảy tâm về con trỏ)
    {
        roomcraft::math::Ray startRay, endRay;
        roomcraft::math::Ray::create({0.50f, 10.0f, -0.10f}, {0.0f, -1.0f, 0.0f}, startRay);
        roomcraft::math::Ray::create({1.50f, 10.0f,  0.90f}, {0.0f, -1.0f, 0.0f}, endRay);

        const bool began = roomcraft::systems::TransformController::beginMove(scene, objId, startRay, {400, 300}, state);
        const bool moved = roomcraft::systems::TransformController::updateMove(scene, endRay, {480, 380}, state);
        const auto pos = scene.findObject(objId)->transform.position;
        roomcraft::systems::TransformController::confirmInteraction(state);

        // Offset (1.0, 0, 1.0) cộng từ vị trí gốc (0.20, 0.75, -0.30) -> (1.20, 0.75, 0.70)
        check(began && moved && approx(pos.x, 1.20f) && approx(pos.y, 0.75f) && approx(pos.z, 0.70f),
              "TC-M5-27 Dragging preserves Y=0.75m elevation & grab offset without jump", failures);
    }

    // TC-M5-28: Grid Snapping (tính từ target chưa snap, xử lý đúng tọa độ âm, giữ nguyên Y)
    {
        state.gridSnapEnabled = true;
        roomcraft::math::Ray startRay, negRay;
        roomcraft::math::Ray::create({1.20f, 10.0f, 0.70f}, {0.0f, -1.0f, 0.0f}, startRay);
        roomcraft::math::Ray::create({-1.18f, 10.0f, -0.77f}, {0.0f, -1.0f, 0.0f}, negRay);

        roomcraft::systems::TransformController::beginMove(scene, objId, startRay, {400, 300}, state);
        roomcraft::systems::TransformController::updateMove(scene, negRay, {200, 200}, state);
        const auto pos = scene.findObject(objId)->transform.position;
        roomcraft::systems::TransformController::confirmInteraction(state);
        state.gridSnapEnabled = false;

        // Grid spacing = 0.5m -> (-1.18 -> -1.0, -0.77 -> -1.0, Y giữ nguyên 0.75)
        check(approx(pos.x, -1.0f) && approx(pos.y, 0.75f) && approx(pos.z, -1.0f),
              "TC-M5-28 Grid Snapping rounds negative XZ to 0.5m grid and preserves Y", failures);
    }

    // TC-M5-29: Interactive Rotation (90°/180° + angle snap 15°) & TC-M5-31 Cancel restores initial transform
    {
        const auto beforeRot = scene.findObject(objId)->transform;
        state.gridSnapEnabled = true;
        state.angleSnapDegrees = 15.0f;
        roomcraft::systems::TransformController::beginRotate(scene, objId, {100.0f, 100.0f}, state);
        // 150 px * 0.6 deg/px = 90 deg
        roomcraft::systems::TransformController::updateRotate(scene, {250.0f, 100.0f}, state);
        const float yaw90 = scene.findObject(objId)->transform.rotationDegrees.y;
        // 300 px * 0.6 deg/px = 180 deg
        roomcraft::systems::TransformController::updateRotate(scene, {400.0f, 100.0f}, state);
        const float yaw180 = scene.findObject(objId)->transform.rotationDegrees.y;
        const bool cancelled = roomcraft::systems::TransformController::cancelInteraction(scene, state);
        const float yawRestored = scene.findObject(objId)->transform.rotationDegrees.y;
        state.gridSnapEnabled = false;

        check(approx(yaw90, 90.0f) && approx(yaw180, 180.0f) && cancelled &&
              approx(yawRestored, beforeRot.rotationDegrees.y) && state.isIdle(),
              "TC-M5-29/31 Interactive rotation (90/180 deg, 15 deg snap) & Cancel restore", failures);
    }

    // TC-M5-30: Interactive Scaling (always strictly positive, clamped [0.1, 10.0], never 0/neg/NaN)
    {
        roomcraft::systems::TransformController::beginScale(scene, objId, {200.0f, 200.0f}, state);
        roomcraft::systems::TransformController::updateScale(scene, {300.0f, 200.0f}, state);
        const auto sUp = scene.findObject(objId)->transform.scale;
        roomcraft::systems::TransformController::updateScale(scene, {-5000.0f, 200.0f}, state);
        const auto sMin = scene.findObject(objId)->transform.scale;
        roomcraft::systems::TransformController::cancelInteraction(scene, state);

        check(sUp.x > 1.2f && approx(sUp.x, sUp.y) && approx(sUp.y, sUp.z) &&
              sMin.x >= roomcraft::systems::TransformController::kMinScale - 1e-4f && sMin.x > 0.0f,
              "TC-M5-30 Interactive uniform scaling clamped positive [0.1, 10.0]", failures);
    }

    // TC-M5-16: Locked object policy (selectable, but TransformController refuses edit with warning)
    {
        scene.findObject(objId)->locked = true;
        std::string err;
        const bool blocked = !roomcraft::systems::TransformController::beginRotate(scene, objId, {0, 0}, state, &err);
        scene.findObject(objId)->locked = false;
        check(blocked && !err.empty() && state.isIdle(),
              "TC-M5-16 Locked object blocks interactive transformation cleanly", failures);
    }

    // TC-M5-32/33/34/35/37: InputManager one-shot edge detection, GUI capture routing, Ctrl+D duplicate & Delete safely
    {
        roomcraft::core::InputManager input;
        roomcraft::editor::ViewportInputContext ctx;
        ctx.viewportRect = {200.0f, 100.0f, 1000.0f, 700.0f, 800.0f, 600.0f};
        ctx.viewportImageHovered = false; // chuột đang trên UI panel khác
        ctx.uiWantsTextInput = true;      // người dùng đang gõ ô Search / Rename

        // Phím Delete & 'g' khi đang gõ text KHÔNG được xóa object hay đổi tool
        input.onKeyDown(127, false, false, false);
        input.onKeyDown('g', false, false, false);
        roomcraft::editor::ViewportInteraction::processInput(input, ctx, scene, selection, camCtrl, cam, state, lookup);
        const bool guiBlockedOk = (scene.getObjectCount() == 1) && (scene.findObject(objId) != nullptr);
        input.endFrame();
        input.onKeyUp(127);
        input.onKeyUp('g');

        // Bây giờ không gõ text: nhấn Ctrl+D nhân bản (chọn object mới), giữ phím thêm 1 frame không nhân bản lần 2!
        ctx.uiWantsTextInput = false;
        ctx.viewportImageHovered = true;
        input.onKeyDown('d', true, false, false);
        roomcraft::editor::ViewportInteraction::processInput(input, ctx, scene, selection, camCtrl, cam, state, lookup);
        const std::size_t countAfterDup = scene.getObjectCount();
        const auto dupId = selection.getSelectedObjectId();
        input.endFrame();

        // Giữ phím 'd' (OS auto-repeat gửi lại onKeyDown khi held==true) -> KHÔNG được nhân bản lần 2
        input.onKeyDown('d', true, false, false);
        roomcraft::editor::ViewportInteraction::processInput(input, ctx, scene, selection, camCtrl, cam, state, lookup);
        const bool noAutoRepeatDup = (scene.getObjectCount() == 2) && (countAfterDup == 2) && (dupId != objId);
        input.endFrame();
        input.onKeyUp('d');

        // Nhấn Delete xóa object đang chọn an toàn & hủy selection + hồi phục nếu đang interacting
        roomcraft::systems::TransformController::beginRotate(scene, dupId, {400, 300}, state);
        input.onKeyDown(127, false, false, false);
        roomcraft::editor::ViewportInteraction::processInput(input, ctx, scene, selection, camCtrl, cam, state, lookup);
        const bool safeDeleteOk = (scene.getObjectCount() == 1) && !selection.hasSelection() && state.isIdle();
        input.endFrame();
        input.onKeyUp(127);

        check(guiBlockedOk && noAutoRepeatDup && safeDeleteOk,
              "TC-M5-32..35/37 GUI text-input capture, one-shot Ctrl+D duplicate & safe Delete during active interaction",
              failures);
    }

    return failures;
}
