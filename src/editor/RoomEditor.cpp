#include "roomcraft/editor/RoomEditor.hpp"

#include "roomcraft/editor/EditorIcons.hpp"
#include "roomcraft/systems/PlacementSystem.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace roomcraft::editor {

void RoomEditor::initialize(assets::AssetManager& assetManager, graphics::Camera& camera)
{
    applyPresetImmediate(scene::ScenePresetType::ModernBedroom, assetManager, camera);
    m_sceneDirty = false;
    m_notifications.clear();
}

std::string RoomEditor::getActiveSceneDisplayName() const
{
    if (m_isCustomEmptyRoom) {
        return "Custom Room";
    }
    switch (m_activePreset) {
        case scene::ScenePresetType::ModernBedroom:        return "Modern Bedroom";
        case scene::ScenePresetType::GamingWorkspace:      return "Gaming Workspace";
        case scene::ScenePresetType::MinimalistLivingRoom: return "Minimalist Living Room";
        default:                                           return "Room Design";
    }
}

void RoomEditor::applyPresetImmediate(scene::ScenePresetType preset,
                                      assets::AssetManager& assetManager,
                                      graphics::Camera& camera)
{
    m_interactionState.resetToIdle();

    std::string err;
    if (!scene::ScenePresets::applyPreset(preset, m_scene, assetManager.getCatalog(), &err)) {
        m_notifications.push(NotificationLevel::Error, "Preset failed: " + err, 4.5f);
        return;
    }

    m_activePreset = preset;
    m_isCustomEmptyRoom = false;
    m_roomDraft = m_scene.getRoom().getSettings();
    m_selection.clearSelection();
    if (!m_scene.getObjects().empty()) {
        m_selection.selectObject(m_scene.getObjects().front().id);
    }
    m_renameSyncedObjectId = scene::kInvalidObjectId;
    m_sceneDirty = false;
    m_interactionState.warningMessage.clear();
    m_statusMessage = "Scene ready";
    m_notifications.push(NotificationLevel::Success, "Loaded " + getActiveSceneDisplayName());

    setPerspective3DCamera(camera);
}

void RoomEditor::createNewEmptyRoomImmediate(graphics::Camera& camera)
{
    m_interactionState.resetToIdle();
    m_scene.clearObjects();
    m_scene.getRoom().resetToDefault();
    m_isCustomEmptyRoom = true;
    m_roomDraft = m_scene.getRoom().getSettings();
    m_selection.clearSelection();
    m_renameSyncedObjectId = scene::kInvalidObjectId;
    m_sceneDirty = false;
    m_interactionState.warningMessage.clear();
    m_statusMessage = "New empty room ready";
    m_notifications.push(NotificationLevel::Info, "Created new room (5.0 x 6.0 x 3.0 m)");
    setPerspective3DCamera(camera);
}

void RoomEditor::requestNewEmptyRoom(graphics::Camera& camera)
{
    m_interactionState.resetToIdle();
    if (m_sceneDirty && m_scene.getObjectCount() > 0) {
        m_pendingNewEmptyRoom = true;
        m_openConfirmSceneModal = true;
    } else {
        createNewEmptyRoomImmediate(camera);
    }
}

void RoomEditor::requestPresetChange(scene::ScenePresetType preset,
                                     assets::AssetManager& assetManager,
                                     graphics::Camera& camera)
{
    m_interactionState.resetToIdle();
    if (m_sceneDirty && m_scene.getObjectCount() > 0) {
        m_pendingPreset = preset;
        m_pendingNewEmptyRoom = false;
        m_openConfirmSceneModal = true;
    } else {
        applyPresetImmediate(preset, assetManager, camera);
    }
}

void RoomEditor::setActiveTool(systems::TransformMode mode)
{
    m_interactionState.activeTool = mode;
    m_interactionState.axisConstraint = systems::AxisConstraint::None;
    m_statusMessage = std::string("Tool: ") + systems::transformModeToString(mode);
}

void RoomEditor::duplicateSelectedObject()
{
    if (!m_selection.hasSelection()) {
        return;
    }
    std::string err;
    const auto newId = m_scene.duplicateObject(m_selection.getSelectedObjectId(), glm::vec3(0.45f, 0.0f, 0.45f), &err);
    if (scene::isValidObjectId(newId)) {
        m_selection.selectObject(newId);
        m_sceneDirty = true;
        m_statusMessage = "Duplicated object #" + std::to_string(newId);
        m_notifications.push(NotificationLevel::Success, "Duplicated object (#" + std::to_string(newId) + ")");
    } else {
        m_notifications.push(NotificationLevel::Error, err);
    }
}

void RoomEditor::requestDeleteSelectedObject()
{
    if (!m_selection.hasSelection()) {
        return;
    }
    const auto id = m_selection.getSelectedObjectId();
    const auto* obj = m_scene.findObject(id);
    if (obj == nullptr) {
        m_selection.clearSelection();
        return;
    }
    if (obj->locked) {
        const std::string msg = "Object '" + obj->name + "' is locked and cannot be deleted.";
        m_interactionState.warningMessage = msg;
        m_notifications.push(NotificationLevel::Warning, msg);
        return;
    }
    m_pendingDeleteObjectId = id;
    m_openConfirmDeleteModal = true;
}

void RoomEditor::resetSelectedTransform()
{
    if (!m_selection.hasSelection()) {
        return;
    }
    const auto* obj = m_scene.findObject(m_selection.getSelectedObjectId());
    if (obj == nullptr) {
        return;
    }
    if (obj->locked) {
        m_notifications.push(NotificationLevel::Warning, "Cannot reset transform of locked object.");
        return;
    }
    scene::Transform resetTr;
    resetTr.position = glm::vec3(0.0f, (obj->assetId == "blender-carpet") ? 0.003f : 0.0f, 0.0f);
    if (m_scene.setObjectTransform(obj->id, resetTr)) {
        m_sceneDirty = true;
        m_notifications.push(NotificationLevel::Info, "Reset transform for '" + obj->name + "'");
    }
}

void RoomEditor::focusRoomSettingsTab()
{
    m_rightBottomTab = 1;
    m_requestRoomTabFocus = true;
}

void RoomEditor::setWallVisibilityMode(scene::WallVisibilityMode mode)
{
    m_roomDraft.wallVisibilityMode = mode;
    std::string err;
    if (m_scene.getRoom().applySettings(m_roomDraft, &err)) {
        m_sceneDirty = true;
        m_statusMessage = std::string("Walls: ") + scene::wallVisibilityModeToString(mode);
    } else {
        m_notifications.push(NotificationLevel::Error, err);
    }
}

void RoomEditor::toggleCeilingVisibility()
{
    m_roomDraft.wallVisibilityMode = scene::WallVisibilityMode::Manual;
    m_roomDraft.showCeiling = !m_scene.getRoom().getSettings().isCeilingVisible();
    std::string err;
    if (m_scene.getRoom().applySettings(m_roomDraft, &err)) {
        m_sceneDirty = true;
    }
}

void RoomEditor::resetLayout()
{
    m_layout.resetDefaults();
    m_notifications.push(NotificationLevel::Info, "Workspace layout reset to defaults.");
}

void RoomEditor::setUiScale(float scale)
{
    m_layout.setUiScale(scale);
}

void RoomEditor::fitCameraToRoom(graphics::Camera& camera)
{
    m_cameraController.fitBoundingBox(m_scene.getRoom().getExteriorBounds(), camera, 1.22f);
}

void RoomEditor::fitCameraToSelected(assets::AssetManager& assetManager, graphics::Camera& camera)
{
    if (!m_selection.hasSelection()) {
        fitCameraToRoom(camera);
        return;
    }
    const auto* obj = m_scene.findObject(m_selection.getSelectedObjectId());
    if (obj == nullptr) {
        fitCameraToRoom(camera);
        return;
    }
    auto gpuAsset = assetManager.getOrLoadAsset(obj->assetId, nullptr);
    if (!gpuAsset) {
        return;
    }
    const math::BoundingBox worldBox = obj->getWorldBounds(gpuAsset->cpuData.localBounds);
    if (worldBox.isValid()) {
        m_cameraController.fitBoundingBox(worldBox, camera, 1.35f);
    }
}

void RoomEditor::setPerspective3DCamera(graphics::Camera& camera)
{
    m_cameraController.setPerspectivePreset(m_scene.getRoom().getExteriorBounds(), camera);
}

void RoomEditor::setIsometricCamera(graphics::Camera& camera)
{
    m_cameraController.setIsometricPreset(m_scene.getRoom().getExteriorBounds(), camera);
}

void RoomEditor::setTopViewCamera(graphics::Camera& camera)
{
    m_cameraController.setTopViewPreset(m_scene.getRoom().getExteriorBounds(), camera);
}

void RoomEditor::setFrontViewCamera(graphics::Camera& camera)
{
    m_cameraController.setFrontViewPreset(m_scene.getRoom().getExteriorBounds(), camera);
}

void RoomEditor::addFurnitureToScene(const std::string& assetId, assets::AssetManager& assetManager)
{
    std::string err;
    auto gpuAsset = assetManager.getOrLoadAsset(assetId, &err);
    if (!gpuAsset) {
        m_notifications.push(NotificationLevel::Error, "Asset load failed: " + err, 4.5f);
        return;
    }

    const auto newId = systems::PlacementSystem::placeFurniture(
        m_scene, assetId, assetManager.getCatalog(), &m_selection, m_gridSnapEnabled, &err);
    if (scene::isValidObjectId(newId)) {
        m_sceneDirty = true;
        m_rightBottomTab = 0;
        const std::string display = gpuAsset->cpuData.displayName;
        m_statusMessage = "Added '" + display + "'";
        m_notifications.push(NotificationLevel::Success, "Added '" + display + "' (#" + std::to_string(newId) + ")");
    } else {
        m_notifications.push(NotificationLevel::Error, err, 4.0f);
    }
}

void RoomEditor::renderSceneToFramebuffer(assets::AssetManager& assetManager,
                                          graphics::Camera& camera,
                                          graphics::Framebuffer& framebuffer,
                                          graphics::Renderer& renderer,
                                          graphics::Shader& meshShader,
                                          graphics::Shader& gridShader,
                                          const graphics::Mesh& axisXMesh,
                                          const graphics::Mesh& axisYMesh,
                                          const graphics::Mesh& axisZMesh,
                                          const glm::vec4& clearColor,
                                          float faceContrast)
{
    if (!framebuffer.isComplete()) {
        return;
    }

    m_selection.reconcileWithScene(m_scene);
    m_interactionState.abortIfInvalid(m_scene);

    graphics::SceneRenderOptions opts;
    opts.wireframe = wireframeMode;
    opts.depthTest = m_depthTest;
    opts.showGrid = showGrid;
    opts.showAxes = showAxes;
    opts.showSelectedBoundingBox = showSelectedBBox;
    opts.showMoveGizmo = showMoveGizmo && (m_interactionState.activeTool == systems::TransformMode::Move);
    opts.activeGizmoAxis = (m_interactionState.phase == systems::InteractionPhase::Moving)
                               ? m_interactionState.axisConstraint
                               : systems::AxisConstraint::None;
    opts.hoveredGizmoAxis = m_interactionState.hoveredGizmoAxis;
    opts.faceContrast = faceContrast;

    framebuffer.bind();
    m_sceneRenderer.renderScene(m_scene,
                                assetManager,
                                m_selection,
                                camera,
                                renderer,
                                meshShader,
                                gridShader,
                                &axisXMesh,
                                &axisYMesh,
                                &axisZMesh,
                                framebuffer.getWidth(),
                                framebuffer.getHeight(),
                                clearColor,
                                opts);
    framebuffer.unbind();
}

bool RoomEditor::renderUiAndProcessInput(int windowWidth,
                                         int windowHeight,
                                         float menuBarHeight,
                                         float deltaSeconds,
                                         const core::InputManager& input,
                                         assets::AssetManager& assetManager,
                                         graphics::ThumbnailRenderer& thumbnailRenderer,
                                         graphics::Shader& meshShader,
                                         graphics::Camera& camera,
                                         graphics::Framebuffer& framebuffer,
                                         float fps,
                                         float frameTimeMs)
{
    m_selection.reconcileWithScene(m_scene);
    m_interactionState.abortIfInvalid(m_scene);
    m_interactionState.gridSnapEnabled = m_gridSnapEnabled;
    m_interactionState.gizmoEnabled = showMoveGizmo;
    m_notifications.update(deltaSeconds);

    const ComputedWorkspaceLayout layout = m_layout.compute(
        static_cast<float>(windowWidth),
        static_cast<float>(windowHeight),
        menuBarHeight);

    renderToolbar(layout.toolbar, assetManager, camera);
    if (layout.leftPanelVisible) {
        renderFurnitureLibraryPanel(layout.leftPanel, assetManager, thumbnailRenderer, meshShader);
    }
    renderViewportPanel(layout.centerViewport, framebuffer, camera);

    // Định tuyến đầu vào ngay sau khi đo xong kích thước ảnh Viewport thực tế
    const auto assetLookup = [&assetManager](const std::string& assetId) -> const assets::AssetData* {
        auto gpuAsset = assetManager.getOrLoadAsset(assetId, nullptr);
        return gpuAsset ? &gpuAsset->cpuData : nullptr;
    };

    const auto outcome = ViewportInteraction::processInput(
        input,
        m_viewportInputCtx,
        m_scene,
        m_selection,
        m_cameraController,
        camera,
        m_interactionState,
        assetLookup);

    if (outcome.sceneModified) {
        m_sceneDirty = true;
    }
    if (!outcome.statusMessage.empty()) {
        m_statusMessage = outcome.statusMessage;
    }
    if (!outcome.errorMessage.empty()) {
        m_notifications.push(NotificationLevel::Warning, outcome.errorMessage, 3.5f);
    }

    if (layout.rightPanelVisible) {
        renderHierarchyPanel(layout.rightTopPanel, assetManager, camera);
        renderPropertiesPanel(layout.rightBottomPanel, assetManager, camera);
    }
    renderSplitters(layout);
    renderStatusBar(layout.statusBar, camera, fps, frameTimeMs);
    renderConfirmationModals(assetManager, camera);

    return outcome.consumedEscape;
}

void RoomEditor::renderToolbar(const PanelRect& rect,
                               assets::AssetManager& assetManager,
                               graphics::Camera& camera)
{
    ImGui::SetNextWindowPos(ImVec2(rect.x, rect.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(rect.width, rect.height), ImGuiCond_Always);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    const float btnH = 28.0f;
    const float padY = std::max(2.0f, std::floor((rect.height - btnH) * 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(1.0f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, padY));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, tokens::kSurface);
    ImGui::Begin("##MainToolbar", nullptr, flags);

    auto drawDivider = [&]() {
        ImGui::SameLine(0.0f, 7.0f);
        const ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(p.x, p.y + 4.0f),
            ImVec2(p.x, p.y + btnH - 4.0f),
            EditorTheme::toU32(tokens::kBorder, 0.85f),
            1.0f);
        ImGui::Dummy(ImVec2(1.0f, btnH));
        ImGui::SameLine(0.0f, 7.0f);
    };

    // Nhóm 1: Project
    if (EditorIcons::toolbarButton("tb_new", EditorIcon::NewRoom, "New", nullptr, false, "Create empty room")) {
        requestNewEmptyRoom(camera);
    }
    drawDivider();

    // Nhóm 2: Interaction Tools
    if (EditorIcons::toolbarButton("tb_sel", EditorIcon::Select, "Select", "Q",
                                   m_interactionState.activeTool == systems::TransformMode::Select, "Select Mode")) {
        setActiveTool(systems::TransformMode::Select);
    }
    ImGui::SameLine(0.0f, 4.0f);
    if (EditorIcons::toolbarButton("tb_mov", EditorIcon::Move, "Move", "G",
                                   m_interactionState.activeTool == systems::TransformMode::Move, "Move Mode & 3-Axis Gizmo")) {
        setActiveTool(systems::TransformMode::Move);
    }
    ImGui::SameLine(0.0f, 4.0f);
    if (EditorIcons::toolbarButton("tb_rot", EditorIcon::Rotate, "Rotate", "R",
                                   m_interactionState.activeTool == systems::TransformMode::Rotate, "Rotate Mode")) {
        setActiveTool(systems::TransformMode::Rotate);
    }
    ImGui::SameLine(0.0f, 4.0f);
    if (EditorIcons::toolbarButton("tb_scl", EditorIcon::Scale, "Scale", "S",
                                   m_interactionState.activeTool == systems::TransformMode::Scale, "Scale Mode")) {
        setActiveTool(systems::TransformMode::Scale);
    }
    drawDivider();

    // Nhóm 3: Snap, Gizmo & Focus
    if (EditorIcons::toolbarButton("tb_snap", EditorIcon::Snap, "Snap", nullptr, m_gridSnapEnabled, "Toggle Grid & 15 deg Angle Snapping")) {
        m_gridSnapEnabled = !m_gridSnapEnabled;
        m_notifications.push(NotificationLevel::Info, m_gridSnapEnabled ? "Grid Snapping enabled" : "Grid Snapping disabled", 2.0f);
    }
    ImGui::SameLine(0.0f, 4.0f);
    if (EditorIcons::toolbarButton("tb_gizmo", EditorIcon::Move, "Gizmo", nullptr, showMoveGizmo, "Toggle 3-Axis Move Gizmo")) {
        showMoveGizmo = !showMoveGizmo;
        m_notifications.push(NotificationLevel::Info, showMoveGizmo ? "3-Axis Move Gizmo enabled" : "3-Axis Move Gizmo hidden", 2.0f);
    }
    ImGui::SameLine(0.0f, 4.0f);
    if (EditorIcons::toolbarButton("tb_focus", EditorIcon::Focus, "Fit", "F", false, "Focus Selected Object (or Fit Room)")) {
        fitCameraToSelected(assetManager, camera);
    }
    drawDivider();

    // Nhóm 4: Camera Views (nhận diện đúng Perspective 3D vs Top 2D vs Isometric)
    const bool isOrtho = (camera.getProjectionMode() == graphics::ProjectionMode::Orthographic);
    const glm::vec3 camDir = glm::normalize(camera.getTarget() - camera.getPosition());
    const bool isTopView = isOrtho && (std::fabs(camDir.y) > 0.94f);
    const bool isIsoView = isOrtho && !isTopView;

    if (EditorIcons::toolbarButton("tb_3d", EditorIcon::Camera, "3D", "2", !isOrtho, "Perspective 3D View")) {
        setPerspective3DCamera(camera);
    }
    ImGui::SameLine(0.0f, 4.0f);
    if (EditorIcons::toolbarButton("tb_top", EditorIcon::Grid, "Top", "1", isTopView, "Orthographic Top View")) {
        setTopViewCamera(camera);
    }
    ImGui::SameLine(0.0f, 4.0f);
    if (EditorIcons::toolbarButton("tb_iso", EditorIcon::Preset, "Iso", "3", isIsoView, "Isometric Orthographic View")) {
        setIsometricCamera(camera);
    }

    // Nhóm bên phải: Room Presets dropdown kèm mũi tên vector
    const float presetBtnW = 132.0f;
    if (ImGui::GetWindowWidth() - ImGui::GetCursorPosX() > presetBtnW + 16.0f) {
        ImGui::SameLine(ImGui::GetWindowWidth() - presetBtnW - 10.0f);
        if (ImGui::Button("Room Presets   ", ImVec2(presetBtnW, btnH))) {
            ImGui::OpenPopup("##ToolbarPresetsPopup");
        }
        const ImVec2 bMin = ImGui::GetItemRectMin();
        const ImVec2 bMax = ImGui::GetItemRectMax();
        const float cx = bMax.x - 14.0f;
        const float cy = (bMin.y + bMax.y) * 0.5f;
        ImGui::GetWindowDrawList()->AddTriangleFilled(
            ImVec2(cx - 4.0f, cy - 2.0f),
            ImVec2(cx + 4.0f, cy - 2.0f),
            ImVec2(cx, cy + 3.0f),
            EditorTheme::toU32(tokens::kTextSecondary));

        if (ImGui::BeginPopup("##ToolbarPresetsPopup")) {
            if (ImGui::MenuItem("Modern Bedroom (5x6x3 m)")) {
                requestPresetChange(scene::ScenePresetType::ModernBedroom, assetManager, camera);
            }
            if (ImGui::MenuItem("Gaming Workspace (4x5x3 m)")) {
                requestPresetChange(scene::ScenePresetType::GamingWorkspace, assetManager, camera);
            }
            if (ImGui::MenuItem("Minimalist Living Room (6x7x3 m)")) {
                requestPresetChange(scene::ScenePresetType::MinimalistLivingRoom, assetManager, camera);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("New Empty Room")) {
                requestNewEmptyRoom(camera);
            }
            ImGui::EndPopup();
        }
    }

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

void RoomEditor::renderFurnitureLibraryPanel(const PanelRect& rect,
                                             assets::AssetManager& assetManager,
                                             graphics::ThumbnailRenderer& thumbnailRenderer,
                                             graphics::Shader& meshShader)
{
    ImGui::SetNextWindowPos(ImVec2(rect.x, rect.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(rect.width, rect.height), ImGuiCond_Always);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
    ImGui::Begin("##FurnitureLibraryPanel", nullptr, flags);

    // Tiêu đề + nút chuyển Grid / List căn sát lề phải gọn gàng
    ImGui::TextColored(tokens::kTextPrimary, "FURNITURE LIBRARY");
    const char* viewBtnLabel = m_libraryGridView ? "List" : "Grid";
    const float viewBtnW = ImGui::CalcTextSize(viewBtnLabel).x + 16.0f;
    ImGui::SameLine(std::max(140.0f, ImGui::GetWindowContentRegionMax().x - viewBtnW));
    if (ImGui::SmallButton(viewBtnLabel)) {
        m_libraryGridView = !m_libraryGridView;
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::SetTooltip("Toggle thumbnail grid / compact list view");
    }

    // Thanh tìm kiếm
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##LibSearch", "Search furniture...", m_librarySearchBuf.data(), m_librarySearchBuf.size());

    // Bộ lọc Category & Source
    static constexpr const char* kCatItems[] = {"All Categories", "Seating", "Tables", "Storage", "Lighting", "Beds", "Electronics", "Decor"};
    static constexpr const char* kSrcItems[] = {"All", "C++", "Blender"};
    const float srcW = 78.0f;
    ImGui::SetNextItemWidth(std::max(80.0f, ImGui::GetContentRegionAvail().x - srcW - 6.0f));
    ImGui::Combo("##LibCat", &m_libraryCategoryFilter, kCatItems, 8);
    ImGui::SameLine(0.0f, 6.0f);
    ImGui::SetNextItemWidth(srcW);
    ImGui::Combo("##LibSrc", &m_librarySourceFilter, kSrcItems, 3);

    std::optional<assets::AssetSourceType> srcFilter;
    if (m_librarySourceFilter == 1) srcFilter = assets::AssetSourceType::Procedural;
    if (m_librarySourceFilter == 2) srcFilter = assets::AssetSourceType::Imported;
    std::optional<assets::AssetCategory> catFilter;
    if (m_libraryCategoryFilter > 0) {
        catFilter = static_cast<assets::AssetCategory>(m_libraryCategoryFilter - 1);
    }

    const auto entries = assetManager.getCatalog().filter(srcFilter, catFilter, m_librarySearchBuf.data());

    // Helper cắt gọn tiêu đề bằng dấu "..." nếu vượt quá chiều rộng thẻ (tránh tràn mép)
    auto fitLabelToWidth = [](const std::string& text, float maxW) -> std::string {
        if (ImGui::CalcTextSize(text.c_str()).x <= maxW) {
            return text;
        }
        std::string trimmed = text;
        while (trimmed.size() > 3) {
            trimmed.pop_back();
            const std::string candidate = trimmed + "...";
            if (ImGui::CalcTextSize(candidate.c_str()).x <= maxW) {
                return candidate;
            }
        }
        return text;
    };

    // Danh sách thẻ Thumbnail thật (FBO 256x256) - luôn giữ thanh cuộn dọc ổn định để 2 cột không xê dịch
    const float footerH = 44.0f;
    ImGui::BeginChild("##LibraryCardsScroll", ImVec2(0.0f, -footerH), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    if (entries.empty()) {
        ImGui::Spacing();
        ImGui::TextDisabled("No matching furniture assets.");
    } else if (m_libraryGridView) {
        const float availW = std::max(120.0f, ImGui::GetContentRegionAvail().x - 2.0f);
        const int cols = (availW >= 196.0f) ? 2 : 1;
        const float gap = tokens::kSpaceSm;
        const float cardW = std::floor((availW - gap * static_cast<float>(cols - 1)) / static_cast<float>(cols));
        const float padX = 7.0f;
        const float padY = 7.0f;
        const float imgW = std::max(56.0f, cardW - padX * 2.0f);
        const float imgH = std::round(imgW * 0.80f); // Tỷ lệ 5:4 gọn đẹp giúp hiển thị được nhiều thẻ hơn
        const float lineH = ImGui::GetTextLineHeight();
        // Chiều cao thẻ tính chính xác theo chiều cao dòng chữ hiện tại (hỗ trợ cả 100/125/150% scale, KHÔNG cắt chữ)
        const float cardH = padY + imgH + 6.0f + lineH + 3.0f + lineH + padY;

        for (std::size_t i = 0; i < entries.size(); ++i) {
            const auto* entry = entries[i];
            if (i % static_cast<std::size_t>(cols) != 0) {
                ImGui::SameLine(0.0f, gap);
            }

            ImGui::PushID(entry->assetId.c_str());
            const bool selected = (m_selectedLibraryAssetId == entry->assetId);

            ImGui::PushStyleColor(ImGuiCol_ChildBg, selected ? tokens::kSelected : tokens::kPanel);
            ImGui::PushStyleColor(ImGuiCol_Border, selected ? tokens::kPrimary : tokens::kBorder);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padX, padY));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 3.0f));
            ImGui::BeginChild("##card", ImVec2(cardW, cardH), true,
                              ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

            const unsigned int thumbTex = thumbnailRenderer.getOrCreateThumbnail(
                entry->assetId, assetManager, meshShader, graphics::ThumbnailRenderer::kDefaultSize);

            const ImVec2 imgPos = ImGui::GetCursorScreenPos();
            if (thumbTex != 0) {
                const ImTextureID texId = static_cast<ImTextureID>(static_cast<std::uintptr_t>(thumbTex));
                // Cắt nhẹ viền trên/dưới theo tỷ lệ 5:4 để mô hình lấp đầy đẹp khung xem trước
                ImGui::Image(texId, ImVec2(imgW, imgH), ImVec2(0.0f, 0.94f), ImVec2(1.0f, 0.06f));
            } else {
                ImGui::Dummy(ImVec2(imgW, imgH));
            }

            // Khung viền tinh tế quanh ảnh + nhãn nguồn nhỏ góc trên-phải ([C++] / [OBJ])
            ImDrawList* cardDl = ImGui::GetWindowDrawList();
            cardDl->AddRect(imgPos, ImVec2(imgPos.x + imgW, imgPos.y + imgH),
                            EditorTheme::toU32(tokens::kBorder, 0.55f), 4.0f);
            const char* srcBadge = (entry->sourceType == assets::AssetSourceType::Procedural) ? "C++" : "OBJ";
            const ImVec2 badgeSz = ImGui::CalcTextSize(srcBadge);
            const ImVec2 bMin(imgPos.x + imgW - badgeSz.x - 10.0f, imgPos.y + 4.0f);
            const ImVec2 bMax(imgPos.x + imgW - 4.0f, imgPos.y + badgeSz.y + 6.0f);
            cardDl->AddRectFilled(bMin, bMax, IM_COL32(20, 24, 31, 195), 3.0f);
            cardDl->AddText(ImVec2(bMin.x + 3.0f, bMin.y + 1.0f),
                            EditorTheme::toU32(entry->sourceType == assets::AssetSourceType::Procedural
                                                   ? tokens::kPrimary
                                                   : tokens::kSuccess),
                            srcBadge);

            // Bắt sự kiện chọn & double-click trực tiếp trên thẻ
            const bool cardHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
            if (cardHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                m_selectedLibraryAssetId = entry->assetId;
            }
            if (cardHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                m_selectedLibraryAssetId = entry->assetId;
                addFurnitureToScene(entry->assetId, assetManager);
            }

            ImGui::Dummy(ImVec2(0.0f, 1.0f));
            const std::string fittedTitle = fitLabelToWidth(entry->displayName, imgW);
            ImGui::TextUnformatted(fittedTitle.c_str());
            ImGui::TextColored(tokens::kTextSecondary, "%s", assets::assetCategoryToString(entry->category));

            ImGui::EndChild();
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(2);

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
                ImGui::SetTooltip("%s\nCategory: %s | Source: %s\nSize: %.2f x %.2f x %.2f m\nDouble-click to add to room",
                                  entry->displayName.c_str(),
                                  assets::assetCategoryToString(entry->category),
                                  assets::assetSourceTypeToString(entry->sourceType),
                                  entry->defaultDimensions.x,
                                  entry->defaultDimensions.y,
                                  entry->defaultDimensions.z);
            }
            ImGui::PopID();
        }
    } else {
        for (const auto* entry : entries) {
            ImGui::PushID(entry->assetId.c_str());
            const bool selected = (m_selectedLibraryAssetId == entry->assetId);
            if (ImGui::Selectable(entry->displayName.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick)) {
                m_selectedLibraryAssetId = entry->assetId;
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    addFurnitureToScene(entry->assetId, assetManager);
                }
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
                ImGui::SetTooltip("%s (%s) — Double-click to add",
                                  entry->displayName.c_str(),
                                  assets::assetSourceTypeToString(entry->sourceType));
            }
            ImGui::PopID();
        }
    }

    ImGui::EndChild();

    // Nút hành động chính: "+ Add Furniture"
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Button, tokens::kPrimary);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, tokens::kPrimaryHover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, tokens::kPrimaryActive);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.06f, 0.08f, 0.12f, 1.0f));
    const bool hasSelectedLib = (assetManager.getCatalog().findById(m_selectedLibraryAssetId) != nullptr);
    if (!hasSelectedLib) ImGui::BeginDisabled();
    if (ImGui::Button("+ Add Furniture", ImVec2(-1.0f, 30.0f))) {
        addFurnitureToScene(m_selectedLibraryAssetId, assetManager);
    }
    if (!hasSelectedLib) ImGui::EndDisabled();
    ImGui::PopStyleColor(4);

    ImGui::End();
}

void RoomEditor::renderViewportPanel(const PanelRect& rect,
                                     graphics::Framebuffer& framebuffer,
                                     graphics::Camera& camera)
{
    ImGui::SetNextWindowPos(ImVec2(rect.x, rect.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(rect.width, rect.height), ImGuiCond_Always);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, tokens::kBg);
    ImGui::Begin("##3DRoomViewport", nullptr, flags);

    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const int targetW = std::max(64, static_cast<int>(avail.x));
    const int targetH = std::max(64, static_cast<int>(avail.y));

    if (framebuffer.isComplete() && (framebuffer.getWidth() != targetW || framebuffer.getHeight() != targetH)) {
        framebuffer.resize(targetW, targetH);
    }
    if (targetH > 0) {
        camera.setAspectRatio(static_cast<float>(targetW) / static_cast<float>(targetH));
    }

    if (framebuffer.isComplete()) {
        const ImTextureID texId = static_cast<ImTextureID>(static_cast<std::uintptr_t>(framebuffer.getColorTextureId()));
        ImGui::Image(texId, ImVec2(static_cast<float>(targetW), static_cast<float>(targetH)),
                     ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

        const ImVec2 rectMin = ImGui::GetItemRectMin();
        const ImVec2 rectMax = ImGui::GetItemRectMax();
        m_viewportInputCtx.viewportRect.minX = rectMin.x;
        m_viewportInputCtx.viewportRect.minY = rectMin.y;
        m_viewportInputCtx.viewportRect.maxX = rectMax.x;
        m_viewportInputCtx.viewportRect.maxY = rectMax.y;
        m_viewportInputCtx.viewportRect.framebufferWidth = static_cast<float>(framebuffer.getWidth());
        m_viewportInputCtx.viewportRect.framebufferHeight = static_cast<float>(framebuffer.getHeight());
        m_viewportInputCtx.viewportImageHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    } else {
        m_viewportInputCtx.viewportRect = math::ViewportRect{};
        m_viewportInputCtx.viewportImageHovered = false;
    }

    // Lớp phủ gọn trên 3D Viewport (không chặn picking ngoài vùng nút nhỏ)
    renderViewportOverlays(rect, camera);

    m_viewportInputCtx.uiWantsTextInput = ImGui::GetIO().WantTextInput;
    m_viewportInputCtx.uiPopupOpen = ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId);
    m_viewportInputCtx.anyUiWidgetActive = ImGui::IsAnyItemActive();

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void RoomEditor::renderViewportOverlays(const PanelRect& rect, graphics::Camera& camera)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // 1. Góc trên-trái: Pill hiển thị chính xác chế độ góc nhìn hiện tại
    const bool isOrtho = (camera.getProjectionMode() == graphics::ProjectionMode::Orthographic);
    const glm::vec3 camDir = glm::normalize(camera.getTarget() - camera.getPosition());
    const char* viewTitle = "Perspective 3D";
    if (isOrtho) {
        viewTitle = (std::fabs(camDir.y) > 0.94f) ? "Top View (2D Ortho)" : "Isometric View";
    }
    const ImVec2 vtSz = ImGui::CalcTextSize(viewTitle);
    const ImVec2 vtMin(rect.x + 12.0f, rect.y + 8.0f);
    const ImVec2 vtMax(vtMin.x + vtSz.x + 18.0f, vtMin.y + 24.0f);
    dl->AddRectFilled(vtMin, vtMax, IM_COL32(28, 33, 42, 210), 5.0f);
    dl->AddRect(vtMin, vtMax, EditorTheme::toU32(tokens::kBorder, 0.80f), 5.0f);
    dl->AddText(ImVec2(vtMin.x + 9.0f, vtMin.y + (24.0f - vtSz.y) * 0.5f),
                EditorTheme::toU32(tokens::kTextPrimary, 0.95f), viewTitle);

    // 2. Góc trên-phải: Các nút gọn chuyển nhanh Solid/Wireframe & Grid
    const float btnAreaW = 152.0f;
    ImGui::SetCursorScreenPos(ImVec2(rect.x + rect.width - btnAreaW - 12.0f, rect.y + 8.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.16f, 0.21f, 0.86f));
    if (ImGui::Button(wireframeMode ? "Wireframe" : "Solid View", ImVec2(84.0f, 24.0f))) {
        wireframeMode = !wireframeMode;
    }
    if (ImGui::IsItemHovered()) {
        m_viewportInputCtx.viewportImageHovered = false;
    }
    ImGui::SameLine(0.0f, 6.0f);
    if (ImGui::Button(showGrid ? "Grid: On" : "Grid: Off", ImVec2(62.0f, 24.0f))) {
        showGrid = !showGrid;
    }
    if (ImGui::IsItemHovered()) {
        m_viewportInputCtx.viewportImageHovered = false;
    }
    ImGui::PopStyleColor();

    // 3. Góc dưới-trái: Pill hiển thị công cụ & đối tượng đang chọn
    std::string modeBadge = std::string(systems::transformModeToString(m_interactionState.activeTool));
    if (m_selection.hasSelection()) {
        if (const auto* obj = m_scene.findObject(m_selection.getSelectedObjectId())) {
            modeBadge += "  |  " + obj->name;
            if (obj->locked) {
                modeBadge += " (Locked)";
            }
        }
    }
    if (m_interactionState.axisConstraint != systems::AxisConstraint::None) {
        modeBadge += "  |  ";
        modeBadge += systems::axisConstraintToString(m_interactionState.axisConstraint);
    } else if (m_interactionState.activeTool == systems::TransformMode::Move) {
        modeBadge += "  |  XZ Plane";
    }

    const ImVec2 badgeSz = ImGui::CalcTextSize(modeBadge.c_str());
    const ImVec2 pillMin(rect.x + 12.0f, rect.y + rect.height - badgeSz.y - 22.0f);
    const ImVec2 pillMax(pillMin.x + badgeSz.x + 20.0f, pillMin.y + badgeSz.y + 12.0f);
    dl->AddRectFilled(pillMin, pillMax, IM_COL32(28, 33, 42, 215), 5.0f);
    dl->AddRect(pillMin, pillMax, EditorTheme::toU32(tokens::kBorder, 0.85f), 5.0f);
    dl->AddText(ImVec2(pillMin.x + 10.0f, pillMin.y + 6.0f), EditorTheme::toU32(tokens::kTextPrimary), modeBadge.c_str());

    // 4. Góc dưới-phải: Toast Notifications không chặn thao tác
    const auto& toasts = m_notifications.getActive();
    float toastBottomY = rect.y + rect.height - 12.0f;
    for (auto it = toasts.rbegin(); it != toasts.rend(); ++it) {
        const auto& n = *it;
        ImVec4 accent = tokens::kPrimary;
        if (n.level == NotificationLevel::Success) accent = tokens::kSuccess;
        else if (n.level == NotificationLevel::Warning) accent = tokens::kWarning;
        else if (n.level == NotificationLevel::Error) accent = tokens::kError;

        const ImVec2 ts = ImGui::CalcTextSize(n.message.c_str());
        const float tw = std::min(rect.width * 0.65f, ts.x + 28.0f);
        const float th = ts.y + 14.0f;
        const ImVec2 tMin(rect.x + rect.width - tw - 12.0f, toastBottomY - th);
        const ImVec2 tMax(rect.x + rect.width - 12.0f, toastBottomY);

        dl->AddRectFilled(tMin, tMax, IM_COL32(30, 35, 44, 232), 5.0f);
        dl->AddRectFilled(tMin, ImVec2(tMin.x + 4.0f, tMax.y), EditorTheme::toU32(accent), 5.0f);
        dl->AddRect(tMin, tMax, EditorTheme::toU32(accent, 0.70f), 5.0f);
        dl->AddText(ImVec2(tMin.x + 12.0f, tMin.y + 7.0f), EditorTheme::toU32(tokens::kTextPrimary), n.message.c_str());

        toastBottomY -= (th + 6.0f);
    }
}

void RoomEditor::renderHierarchyPanel(const PanelRect& rect,
                                      assets::AssetManager& assetManager,
                                      graphics::Camera& camera)
{
    ImGui::SetNextWindowPos(ImVec2(rect.x, rect.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(rect.width, rect.height), ImGuiCond_Always);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
    ImGui::Begin("##SceneHierarchyPanel", nullptr, flags);

    ImGui::TextColored(tokens::kTextPrimary, "SCENE HIERARCHY");
    char countBuf[24];
    std::snprintf(countBuf, sizeof(countBuf), "%zu items", m_scene.getObjectCount());
    const float countW = ImGui::CalcTextSize(countBuf).x;
    ImGui::SameLine(std::max(140.0f, ImGui::GetWindowContentRegionMax().x - countW));
    ImGui::TextColored(tokens::kTextSecondary, "%s", countBuf);

    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##HierSearch", "Filter scene...", m_hierarchySearchBuf.data(), m_hierarchySearchBuf.size());

    ImGui::BeginChild("##HierarchyList", ImVec2(0.0f, 0.0f), false);

    const std::string rootTitle = getActiveSceneDisplayName();
    if (ImGui::TreeNodeEx(rootTitle.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        const bool roomTabSelected = (m_rightBottomTab == 1 && !m_selection.hasSelection());
        if (ImGui::Selectable("Floor & Walls", roomTabSelected, 0, ImVec2(0.0f, 22.0f))) {
            m_selection.clearSelection();
            m_rightBottomTab = 1;
            m_requestRoomTabFocus = true;
        }

        const std::string filterStr(m_hierarchySearchBuf.data());
        scene::ObjectId pendingDeleteId = scene::kInvalidObjectId;
        scene::ObjectId pendingDuplicateId = scene::kInvalidObjectId;
        scene::ObjectId pendingFocusId = scene::kInvalidObjectId;

        for (const auto& obj : m_scene.getObjects()) {
            if (!filterStr.empty() && obj.name.find(filterStr) == std::string::npos &&
                obj.assetId.find(filterStr) == std::string::npos) {
                continue;
            }

            // Dùng Stable ObjectId làm ImGui ID để các object trùng tên không bao giờ xung đột UI
            ImGui::PushID(static_cast<int>(obj.id));

            const bool selected = m_selection.isSelected(obj.id);
            const float rowButtonsW = 50.0f;
            const float rowStartX = ImGui::GetCursorPosX();
            const float availRowW = ImGui::GetContentRegionAvail().x;
            const float selW = std::max(60.0f, availRowW - rowButtonsW - 4.0f);

            if (selected) {
                ImGui::PushStyleColor(ImGuiCol_Header, tokens::kSelected);
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, tokens::kSelected);
            }
            if (ImGui::Selectable(obj.name.c_str(), selected, ImGuiSelectableFlags_AllowOverlap, ImVec2(selW, 23.0f))) {
                m_selection.selectObject(obj.id);
                m_rightBottomTab = 0;
            }
            if (selected) {
                ImGui::PopStyleColor(2);
            }

            // Context menu chuột phải đầy đủ hành động
            if (ImGui::BeginPopupContextItem("##HierCtx")) {
                m_selection.selectObject(obj.id);
                if (ImGui::MenuItem("Focus Selected", "F")) {
                    pendingFocusId = obj.id;
                }
                if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
                    pendingDuplicateId = obj.id;
                }
                if (ImGui::MenuItem(obj.visible ? "Hide" : "Show")) {
                    m_scene.setObjectVisibility(obj.id, !obj.visible);
                    m_sceneDirty = true;
                }
                if (ImGui::MenuItem(obj.locked ? "Unlock" : "Lock")) {
                    if (auto* mutObj = m_scene.findObject(obj.id)) {
                        mutObj->locked = !mutObj->locked;
                        m_sceneDirty = true;
                        if (mutObj->locked) m_interactionState.abortIfInvalid(m_scene);
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Delete", "Del", false, !obj.locked)) {
                    pendingDeleteId = obj.id;
                }
                ImGui::EndPopup();
            }

            ImGui::SameLine(rowStartX + availRowW - rowButtonsW);
            if (EditorIcons::smallIconButton("eye",
                                             obj.visible ? EditorIcon::EyeOpen : EditorIcon::EyeClosed,
                                             false,
                                             obj.visible ? "Hide object" : "Show object",
                                             obj.visible ? tokens::kTextPrimary : tokens::kTextSecondary)) {
                m_scene.setObjectVisibility(obj.id, !obj.visible);
                m_sceneDirty = true;
            }
            ImGui::SameLine(0.0f, 4.0f);
            if (EditorIcons::smallIconButton("lock",
                                             obj.locked ? EditorIcon::LockClosed : EditorIcon::LockOpen,
                                             obj.locked,
                                             obj.locked ? "Unlock object" : "Lock object",
                                             obj.locked ? tokens::kWarning : tokens::kTextSecondary)) {
                if (auto* mutObj = m_scene.findObject(obj.id)) {
                    mutObj->locked = !mutObj->locked;
                    m_sceneDirty = true;
                    if (mutObj->locked) m_interactionState.abortIfInvalid(m_scene);
                }
            }

            ImGui::PopID();
        }

        if (scene::isValidObjectId(pendingFocusId)) {
            m_selection.selectObject(pendingFocusId);
            fitCameraToSelected(assetManager, camera);
        }
        if (scene::isValidObjectId(pendingDuplicateId)) {
            m_selection.selectObject(pendingDuplicateId);
            duplicateSelectedObject();
        }
        if (scene::isValidObjectId(pendingDeleteId)) {
            m_selection.selectObject(pendingDeleteId);
            requestDeleteSelectedObject();
        }

        ImGui::TreePop();
    }

    ImGui::EndChild();
    ImGui::End();
}

bool RoomEditor::renderAxisDragFloat3(const char* label,
                                      glm::vec3& vec,
                                      float speed,
                                      float minVal,
                                      float maxVal,
                                      const char* fmt,
                                      bool* outDeactivatedAfterEdit)
{
    bool changed = false;
    bool deactivated = false;

    ImGui::PushID(label);
    if (label != nullptr && label[0] != '\0') {
        ImGui::TextColored(tokens::kTextSecondary, "%s", label);
    }

    const float avail = ImGui::GetContentRegionAvail().x;
    const float colGap = 6.0f;
    const float colW = std::floor((avail - colGap * 2.0f) / 3.0f);
    const float badgeW = 20.0f;
    const float rowH = ImGui::GetFrameHeight();
    const char* axisNames[3] = {"X", "Y", "Z"};
    const ImVec4 axisCols[3] = {tokens::kAxisX, tokens::kAxisY, tokens::kAxisZ};

    for (int i = 0; i < 3; ++i) {
        if (i > 0) ImGui::SameLine(0.0f, colGap);
        ImGui::PushID(i);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(axisCols[i].x, axisCols[i].y, axisCols[i].z, 0.22f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(axisCols[i].x, axisCols[i].y, axisCols[i].z, 0.30f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(axisCols[i].x, axisCols[i].y, axisCols[i].z, 0.36f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(axisCols[i].x, axisCols[i].y, axisCols[i].z, 0.55f));
        ImGui::PushStyleColor(ImGuiCol_Text, axisCols[i]);
        ImGui::Button(axisNames[i], ImVec2(badgeW, rowH));
        ImGui::PopStyleColor(5);

        ImGui::SameLine(0.0f, 2.0f);
        ImGui::SetNextItemWidth(std::max(36.0f, colW - badgeW - 2.0f));
        if (ImGui::DragFloat("##val", &vec[i], speed, minVal, maxVal, fmt)) {
            changed = true;
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            deactivated = true;
        }

        ImGui::PopID();
    }

    if (outDeactivatedAfterEdit != nullptr) {
        *outDeactivatedAfterEdit = deactivated;
    }
    ImGui::PopID();
    return changed;
}

void RoomEditor::renderPropertiesPanel(const PanelRect& rect,
                                       assets::AssetManager& assetManager,
                                       graphics::Camera& camera)
{
    ImGui::SetNextWindowPos(ImVec2(rect.x, rect.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(rect.width, rect.height), ImGuiCond_Always);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
    ImGui::Begin("##PropertiesInspectorPanel", nullptr, flags);

    if (ImGui::BeginTabBar("##InspectorTabs")) {
        const ImGuiTabItemFlags objFlags = (!m_requestRoomTabFocus && m_rightBottomTab == 0) ? 0 : 0;
        const ImGuiTabItemFlags roomFlags = m_requestRoomTabFocus ? ImGuiTabItemFlags_SetSelected : 0;
        m_requestRoomTabFocus = false;

        if (ImGui::BeginTabItem("PROPERTIES", nullptr, objFlags)) {
            m_rightBottomTab = 0;
            auto* obj = m_selection.hasSelection() ? m_scene.findObject(m_selection.getSelectedObjectId()) : nullptr;

            if (obj == nullptr) {
                ImGui::Spacing();
                ImGui::TextDisabled("No furniture object selected.");
                ImGui::TextWrapped("Click an object in the 3D Viewport or Scene Hierarchy, or open Room Settings.");
                ImGui::Spacing();
                if (ImGui::Button("Open Room Settings", ImVec2(-1.0f, 28.0f))) {
                    focusRoomSettingsTab();
                }
            } else {
                if (m_renameSyncedObjectId != obj->id) {
                    std::snprintf(m_renameBuf.data(), m_renameBuf.size(), "%s", obj->name.c_str());
                    m_renameSyncedObjectId = obj->id;
                }

                const auto* catalogEntry = assetManager.getCatalog().findById(obj->assetId);
                auto gpuAsset = assetManager.getOrLoadAsset(obj->assetId, nullptr);

                // 1. Nhóm Object
                if (ImGui::CollapsingHeader("Object", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 62.0f);
                    ImGui::InputText("##ObjName", m_renameBuf.data(), m_renameBuf.size());
                    ImGui::SameLine(0.0f, 4.0f);
                    if (ImGui::Button("Rename", ImVec2(58.0f, 0.0f))) {
                        std::string err;
                        if (m_scene.renameObject(obj->id, m_renameBuf.data(), &err)) {
                            m_sceneDirty = true;
                            m_notifications.push(NotificationLevel::Success, "Renamed to '" + obj->name + "'");
                        } else {
                            m_notifications.push(NotificationLevel::Error, err);
                        }
                    }
                    ImGui::TextColored(tokens::kTextSecondary, "ID #%llu  |  %s",
                                       static_cast<unsigned long long>(obj->id),
                                       catalogEntry ? assets::assetSourceTypeToString(catalogEntry->sourceType) : "Asset");

                    bool vis = obj->visible;
                    if (ImGui::Checkbox("Visible", &vis)) {
                        m_scene.setObjectVisibility(obj->id, vis);
                        m_sceneDirty = true;
                    }
                    ImGui::SameLine(0.0f, 16.0f);
                    if (ImGui::Checkbox("Locked", &obj->locked)) {
                        m_sceneDirty = true;
                        if (obj->locked) m_interactionState.abortIfInvalid(m_scene);
                    }

                    if (obj->locked) {
                        ImGui::PushStyleColor(ImGuiCol_Text, tokens::kWarning);
                        ImGui::TextWrapped("[Locked] Unlock object to transform or delete.");
                        ImGui::PopStyleColor();
                    }
                }

                // 2. Nhóm Transform (với nhãn X Đỏ / Y Xanh lá / Z Xanh dương)
                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                    if (obj->locked) ImGui::BeginDisabled();

                    scene::Transform edited = obj->transform;
                    bool changed = false;
                    bool posDeactivated = false;

                    const float posSpeed = m_gridSnapEnabled ? 0.10f : 0.05f;
                    changed |= renderAxisDragFloat3("Position (m)", edited.position, posSpeed, -30.0f, 30.0f, "%.2f", &posDeactivated);
                    if (m_gridSnapEnabled && posDeactivated) {
                        edited.position = systems::PlacementSystem::snapToGrid(
                            edited.position, m_scene.getRoom().getSettings().gridSpacing, false);
                        changed = true;
                    }

                    changed |= renderAxisDragFloat3("Rotation (deg)", edited.rotationDegrees, 1.0f, -360.0f, 360.0f, "%.1f");

                    ImGui::Spacing();
                    ImGui::TextColored(tokens::kTextSecondary, "Scale");
                    const float uniW = ImGui::CalcTextSize("Uniform").x + 28.0f;
                    ImGui::SameLine(std::max(60.0f, ImGui::GetWindowContentRegionMax().x - uniW));
                    ImGui::Checkbox("Uniform", &m_uniformScale);
                    if (m_uniformScale) {
                        float uni = edited.scale.x;
                        ImGui::SetNextItemWidth(-1.0f);
                        if (ImGui::DragFloat("##UniScale", &uni, 0.02f, 0.10f, 10.0f, "Uniform Scale: %.2f")) {
                            edited.scale = glm::vec3(uni, uni, uni);
                            changed = true;
                        }
                    } else {
                        changed |= renderAxisDragFloat3("", edited.scale, 0.02f, 0.10f, 10.0f, "%.2f");
                    }

                    if (changed) {
                        std::string err;
                        if (m_scene.setObjectTransform(obj->id, edited, &err)) {
                            m_sceneDirty = true;
                        } else {
                            m_notifications.push(NotificationLevel::Error, err);
                        }
                    }

                    ImGui::Spacing();
                    if (ImGui::Button("Reset Transform", ImVec2(-1.0f, 26.0f))) {
                        resetSelectedTransform();
                    }
                    if (obj->locked) ImGui::EndDisabled();
                }

                // 3. Nhóm Dimensions
                if (gpuAsset && ImGui::CollapsingHeader("Dimensions", ImGuiTreeNodeFlags_DefaultOpen)) {
                    const glm::vec3 localSz = gpuAsset->cpuData.localBounds.getSize();
                    const math::BoundingBox worldBox = obj->getWorldBounds(gpuAsset->cpuData.localBounds);
                    const glm::vec3 worldSz = worldBox.getSize();
                    ImGui::Text("Asset : %.2f x %.2f x %.2f m", localSz.x, localSz.y, localSz.z);
                    ImGui::Text("World : %.2f x %.2f x %.2f m", worldSz.x, worldSz.y, worldSz.z);

                    if (!m_scene.isBoundsWithinRoom(worldBox)) {
                        ImGui::PushStyleColor(ImGuiCol_Text, tokens::kWarning);
                        ImGui::TextWrapped("[Warning] Exceeds room interior bounds.");
                        ImGui::PopStyleColor();
                    }
                }

                // 4. Nhóm Material (chỉ hiển thị thông tin thật mà renderer M6 hỗ trợ)
                if (gpuAsset && ImGui::CollapsingHeader("Material")) {
                    ImGui::Text("Materials: %zu | Textures: %zu",
                                gpuAsset->cpuData.materials.size(),
                                gpuAsset->cpuData.getUniqueTextureCount());
                    ImGui::TextColored(tokens::kTextSecondary, "Diffuse baseColor + map_Kd active.");
                }

                // 5. Nhóm Geometry (mặc định thu gọn để giữ giao diện gọn gàng)
                if (gpuAsset && ImGui::CollapsingHeader("Geometry")) {
                    ImGui::Text("Meshes    : %zu", gpuAsset->cpuData.subMeshes.size());
                    ImGui::Text("Vertices  : %u", gpuAsset->cpuData.getTotalVertexCount());
                    ImGui::Text("Triangles : %u", gpuAsset->cpuData.getTotalTriangleCount());
                    ImGui::TextColored(tokens::kTextSecondary, "Asset ID  : %s", obj->assetId.c_str());
                }

                // 6. Nhóm Actions
                if (ImGui::CollapsingHeader("Actions", ImGuiTreeNodeFlags_DefaultOpen)) {
                    const float btnW = std::floor((ImGui::GetContentRegionAvail().x - 8.0f) / 3.0f);
                    if (ImGui::Button("Focus", ImVec2(btnW, 26.0f))) {
                        fitCameraToSelected(assetManager, camera);
                    }
                    ImGui::SameLine(0.0f, 4.0f);
                    if (ImGui::Button("Duplicate", ImVec2(btnW, 26.0f))) {
                        duplicateSelectedObject();
                    }
                    ImGui::SameLine(0.0f, 4.0f);
                    if (obj->locked) ImGui::BeginDisabled();
                    ImGui::PushStyleColor(ImGuiCol_Text, tokens::kError);
                    if (ImGui::Button("Delete", ImVec2(btnW, 26.0f))) {
                        requestDeleteSelectedObject();
                    }
                    ImGui::PopStyleColor();
                    if (obj->locked) ImGui::EndDisabled();
                }
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("ROOM SETTINGS", nullptr, roomFlags)) {
            m_rightBottomTab = 1;
            renderRoomSettingsContent(assetManager);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void RoomEditor::renderRoomSettingsContent(assets::AssetManager& /*assetManager*/)
{
    ImGui::TextColored(tokens::kTextSecondary, "Interior Dimensions (meters)");
    ImGui::DragFloat("Width (X)", &m_roomDraft.width, 0.1f, scene::room_limits::kMinWidth, scene::room_limits::kMaxWidth, "%.2f m");
    ImGui::DragFloat("Length (Z)", &m_roomDraft.length, 0.1f, scene::room_limits::kMinLength, scene::room_limits::kMaxLength, "%.2f m");
    ImGui::DragFloat("Height (Y)", &m_roomDraft.height, 0.1f, scene::room_limits::kMinHeight, scene::room_limits::kMaxHeight, "%.2f m");
    ImGui::DragFloat("Wall Thick", &m_roomDraft.wallThickness, 0.01f, scene::room_limits::kMinWallThickness, scene::room_limits::kMaxWallThickness, "%.2f m");
    ImGui::DragFloat("Grid Step", &m_roomDraft.gridSpacing, 0.05f, scene::room_limits::kMinGridSpacing, scene::room_limits::kMaxGridSpacing, "%.2f m");

    ImGui::Separator();
    ImGui::TextColored(tokens::kTextSecondary, "Colors & Walls");
    ImGui::ColorEdit3("Floor Color", &m_roomDraft.floorColor.r);
    ImGui::ColorEdit3("Wall Color", &m_roomDraft.wallColor.r);
    ImGui::ColorEdit3("Ceiling Color", &m_roomDraft.ceilingColor.r);

    static constexpr const char* kVisModes[] = {"All Walls", "Hide Ceiling", "Cutaway View", "Manual"};
    int modeIdx = static_cast<int>(m_roomDraft.wallVisibilityMode);
    if (ImGui::Combo("Wall Mode", &modeIdx, kVisModes, 4)) {
        m_roomDraft.wallVisibilityMode = static_cast<scene::WallVisibilityMode>(modeIdx);
    }
    if (m_roomDraft.wallVisibilityMode == scene::WallVisibilityMode::Manual) {
        ImGui::Checkbox("North (-Z)", &m_roomDraft.showNorthWall);
        ImGui::SameLine();
        ImGui::Checkbox("South (+Z)", &m_roomDraft.showSouthWall);
        ImGui::Checkbox("West (-X)", &m_roomDraft.showWestWall);
        ImGui::SameLine();
        ImGui::Checkbox("East (+X)", &m_roomDraft.showEastWall);
        ImGui::Checkbox("Show Ceiling", &m_roomDraft.showCeiling);
    }
    ImGui::Checkbox("Show Room Grid", &m_roomDraft.showGrid);

    ImGui::Spacing();
    if (ImGui::Button("Apply Changes", ImVec2(124.0f, 26.0f))) {
        std::string err;
        if (m_scene.getRoom().applySettings(m_roomDraft, &err)) {
            m_sceneDirty = true;
            m_notifications.push(NotificationLevel::Success, "Room settings applied.");
        } else {
            m_notifications.push(NotificationLevel::Error, err, 4.0f);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset", ImVec2(76.0f, 26.0f))) {
        m_roomDraft = scene::RoomSettings{};
        std::string err;
        m_scene.getRoom().applySettings(m_roomDraft, &err);
        m_notifications.push(NotificationLevel::Info, "Room settings reset.");
    }

    ImGui::Separator();
    const auto& activeRs = m_scene.getRoom().getSettings();
    ImGui::TextColored(tokens::kTextSecondary, "Floor Area: %.1f m^2  |  Volume: %.1f m^3",
                       activeRs.getFloorArea(), activeRs.getVolume());
}

void RoomEditor::renderSplitters(const ComputedWorkspaceLayout& layout)
{
    // QUAN TRỌNG: WindowMinSize mặc định của Dear ImGui là (32, 32).
    // Phải hạ WindowMinSize xuống (1, 1) cho các cửa sổ splitter mảnh 5px để KHÔNG bao giờ phình ra đè lên panel bên cạnh!
    const ImGuiWindowFlags splitterFlags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav;

    auto handleVertSplitter = [&](const char* id, const PanelRect& r, float& deltaOut) -> bool {
        if (!r.isValid()) return false;
        ImGui::SetNextWindowPos(ImVec2(r.x, r.y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(r.width, r.height), ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(1.0f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, tokens::kBg);

        ImGui::Begin(id, nullptr, splitterFlags);
        ImGui::InvisibleButton("##drag", ImVec2(r.width, r.height));
        const bool hovered = ImGui::IsItemHovered();
        const bool active = ImGui::IsItemActive();

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float midX = std::floor(r.x + r.width * 0.5f);
        const float midY = r.y + r.height * 0.5f;
        if (hovered || active) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            dl->AddRectFilled(ImVec2(r.x + 1.0f, r.y), ImVec2(r.x + r.width - 1.0f, r.y + r.height),
                              EditorTheme::toU32(tokens::kPrimary, active ? 0.90f : 0.48f));
        } else {
            dl->AddLine(ImVec2(midX, r.y), ImVec2(midX, r.y + r.height),
                        EditorTheme::toU32(tokens::kBorder, 0.50f), 1.0f);
            dl->AddRectFilled(ImVec2(midX - 1.0f, midY - 14.0f), ImVec2(midX + 2.0f, midY + 14.0f),
                              EditorTheme::toU32(tokens::kBorder, 0.90f), 1.5f);
        }

        bool dragged = false;
        if (active && std::fabs(ImGui::GetIO().MouseDelta.x) > 0.01f) {
            deltaOut = ImGui::GetIO().MouseDelta.x;
            dragged = true;
        }
        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);
        return dragged;
    };

    float dx = 0.0f;
    if (layout.leftPanelVisible && handleVertSplitter("##LeftSplitter", layout.leftSplitter, dx)) {
        m_layout.setLeftPanelWidth(m_layout.getLeftPanelWidth() + dx / m_layout.getUiScale());
    }
    if (layout.rightPanelVisible && handleVertSplitter("##RightSplitter", layout.rightSplitter, dx)) {
        m_layout.setRightPanelWidth(m_layout.getRightPanelWidth() - dx / m_layout.getUiScale());
    }

    if (layout.rightPanelVisible && layout.rightHorizSplitter.isValid()) {
        const auto& r = layout.rightHorizSplitter;
        ImGui::SetNextWindowPos(ImVec2(r.x, r.y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(r.width, r.height), ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(1.0f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, tokens::kBg);

        ImGui::Begin("##RightHorizSplitter", nullptr, splitterFlags);
        ImGui::InvisibleButton("##hdrag", ImVec2(r.width, r.height));
        const bool hovered = ImGui::IsItemHovered();
        const bool active = ImGui::IsItemActive();

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float midX = r.x + r.width * 0.5f;
        const float midY = std::floor(r.y + r.height * 0.5f);
        if (hovered || active) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            dl->AddRectFilled(ImVec2(r.x, r.y + 1.0f), ImVec2(r.x + r.width, r.y + r.height - 1.0f),
                              EditorTheme::toU32(tokens::kPrimary, active ? 0.90f : 0.48f));
        } else {
            dl->AddLine(ImVec2(r.x, midY), ImVec2(r.x + r.width, midY),
                        EditorTheme::toU32(tokens::kBorder, 0.50f), 1.0f);
            dl->AddRectFilled(ImVec2(midX - 16.0f, midY - 1.0f), ImVec2(midX + 16.0f, midY + 2.0f),
                              EditorTheme::toU32(tokens::kBorder, 0.90f), 1.5f);
        }

        if (active && std::fabs(ImGui::GetIO().MouseDelta.y) > 0.01f) {
            const float totalRightH = std::max(200.0f, layout.rightTopPanel.height + layout.rightBottomPanel.height);
            m_layout.setHierarchyHeightRatio(m_layout.getHierarchyHeightRatio() +
                                             ImGui::GetIO().MouseDelta.y / totalRightH);
        }
        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);
    }
}

void RoomEditor::renderStatusBar(const PanelRect& rect,
                                 const graphics::Camera& camera,
                                 float fps,
                                 float frameTimeMs)
{
    ImGui::SetNextWindowPos(ImVec2(rect.x, rect.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(rect.width, rect.height), ImGuiCond_Always);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    const float padY = std::max(2.0f, std::floor((rect.height - ImGui::GetTextLineHeight()) * 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(1.0f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(tokens::kSpaceMd, padY));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, tokens::kBg);
    ImGui::Begin("##RoomEditorStatusBar", nullptr, flags);

    const auto& rs = m_scene.getRoom().getSettings();
    const char* camStr = (camera.getProjectionMode() == graphics::ProjectionMode::Perspective) ? "Perspective" : "Orthographic";

    std::string selName = "None";
    if (m_selection.hasSelection()) {
        if (const auto* obj = m_scene.findObject(m_selection.getSelectedObjectId())) {
            selName = obj->name;
        }
    }

    if (showPerformanceInStatus) {
        ImGui::TextColored(tokens::kTextSecondary,
                           "Room: %.1f x %.1f x %.1f m  |  Objects: %zu  |  Grid: %.2f m  |  %s  |  Selected: %s  |  %.1f FPS (%.1f ms)",
                           rs.width, rs.length, rs.height, m_scene.getObjectCount(),
                           rs.gridSpacing, camStr, selName.c_str(), fps, frameTimeMs);
    } else {
        ImGui::TextColored(tokens::kTextSecondary,
                           "Room: %.1f x %.1f x %.1f m  |  Objects: %zu  |  Grid: %.2f m  |  %s  |  Selected: %s",
                           rs.width, rs.length, rs.height, m_scene.getObjectCount(),
                           rs.gridSpacing, camStr, selName.c_str());
    }

    // Bên phải: Chấm trạng thái màu xanh + status message ngắn gọn
    const std::string rightText = m_interactionState.isIdle()
                                      ? m_statusMessage
                                      : std::string(systems::interactionPhaseToString(m_interactionState.phase));
    const ImVec2 rightSz = ImGui::CalcTextSize(rightText.c_str());
    const float rightX = rect.width - rightSz.x - 16.0f;
    if (rightX > ImGui::GetCursorPosX() + 20.0f) {
        ImGui::SameLine(rightX);
        const ImVec2 dotPos(rect.x + rightX - 10.0f, rect.y + rect.height * 0.5f);
        ImGui::GetWindowDrawList()->AddCircleFilled(
            dotPos, 4.0f,
            EditorTheme::toU32(m_interactionState.warningMessage.empty() ? tokens::kSuccess : tokens::kWarning));
        ImGui::TextUnformatted(rightText.c_str());
    }

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

void RoomEditor::renderConfirmationModals(assets::AssetManager& assetManager, graphics::Camera& camera)
{
    if (m_openConfirmSceneModal) {
        ImGui::OpenPopup("Confirm Replace Scene");
        m_openConfirmSceneModal = false;
    }
    if (ImGui::BeginPopupModal("Confirm Replace Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("Your current room has unsaved edits. Replace the current scene?");
        ImGui::Spacing();
        if (ImGui::Button("Replace Scene", ImVec2(124.0f, 0.0f))) {
            if (m_pendingNewEmptyRoom) {
                createNewEmptyRoomImmediate(camera);
            } else {
                applyPresetImmediate(m_pendingPreset, assetManager, camera);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(96.0f, 0.0f))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (m_openConfirmDeleteModal) {
        ImGui::OpenPopup("Confirm Delete Object");
        m_openConfirmDeleteModal = false;
    }
    if (ImGui::BeginPopupModal("Confirm Delete Object", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        const auto* obj = m_scene.findObject(m_pendingDeleteObjectId);
        if (obj == nullptr) {
            ImGui::CloseCurrentPopup();
        } else {
            ImGui::Text("Delete '%s' (#%llu) from the room?",
                        obj->name.c_str(), static_cast<unsigned long long>(obj->id));
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.24f, 0.24f, 1.0f));
            if (ImGui::Button("Delete", ImVec2(104.0f, 0.0f))) {
                const std::string nameCopy = obj->name;
                m_scene.removeObject(m_pendingDeleteObjectId);
                m_interactionState.abortIfInvalid(m_scene);
                m_selection.reconcileWithScene(m_scene);
                m_sceneDirty = true;
                m_pendingDeleteObjectId = scene::kInvalidObjectId;
                m_notifications.push(NotificationLevel::Info, "Deleted '" + nameCopy + "'");
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(96.0f, 0.0f))) {
                m_pendingDeleteObjectId = scene::kInvalidObjectId;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
}

void RoomEditor::shutdownGpuResources()
{
    m_interactionState.resetToIdle();
    m_sceneRenderer.destroyGpuResources();
}

} // namespace roomcraft::editor
