#include "roomcraft/editor/RoomEditor.hpp"

#include "roomcraft/core/FileDialog.hpp"
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
    m_projectManager.clearDirty();
    m_notifications.clear();
}

std::string RoomEditor::getActiveSceneDisplayName() const
{
    if (!m_projectManager.getProjectName().empty()) {
        return m_projectManager.getProjectName();
    }
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

std::string RoomEditor::getWindowTitleLabel() const
{
    std::string label = getActiveSceneDisplayName();
    if (m_projectManager.isDirty()) {
        label += " *";
    }
    return label;
}

void RoomEditor::markSceneModified(bool collisionAffected)
{
    m_projectManager.markDirty();
    if (collisionAffected) {
        m_collisionDirty = true;
    }
}

void RoomEditor::recordUndoBeforeEdit(const std::string& description)
{
    m_commandHistory.pushUndo(
        systems::CommandHistory::capture(m_scene, m_selection.getSelectedObjectId(), description));
}

systems::CollisionSystem::LocalBoundsLookupFn RoomEditor::makeBoundsLookup(assets::AssetManager& assetManager) const
{
    return [&assetManager](const std::string& assetId) -> math::BoundingBox {
        auto gpu = assetManager.getOrLoadAsset(assetId, nullptr);
        return gpu ? gpu->cpuData.localBounds : math::BoundingBox{};
    };
}

void RoomEditor::setPlacementValidationMode(systems::PlacementValidationMode mode)
{
    if (m_placementMode != mode) {
        m_placementMode = mode;
        markSceneModified(true);
    }
}

void RoomEditor::setCollisionOptions(const systems::CollisionOptions& opts)
{
    m_collisionOptions = opts;
    m_collisionDirty = true;
}

void RoomEditor::refreshCollisionReport(assets::AssetManager& assetManager)
{
    if (m_placementMode == systems::PlacementValidationMode::Off) {
        m_collisionReport = systems::SceneCollisionReport{};
        m_collisionDirty = false;
        return;
    }
    m_collisionReport = systems::CollisionSystem::validateScene(
        m_scene, makeBoundsLookup(assetManager), m_collisionOptions);
    m_collisionDirty = false;
}

persistence::ProjectData RoomEditor::buildProjectData(const graphics::Camera& camera) const
{
    persistence::ProjectData data;
    data.format = persistence::kProjectFormatId;
    data.schemaVersion = persistence::kCurrentSchemaVersion;
    data.projectName = getActiveSceneDisplayName();
    data.room = m_scene.getRoom().getSettings();
    data.nextObjectId = m_scene.getNextObjectId();
    data.objects = m_scene.getObjects();
    data.lightingPreset = m_lightPreset;
    data.lighting = m_lighting;
    data.renderSettings.msaaSampleIndex = m_sampleIndex;
    data.renderSettings.wireframe = wireframeMode;
    data.renderSettings.showAxes = showAxes;
    data.renderSettings.showSelectedBBox = showSelectedBBox;
    data.renderSettings.showMoveGizmo = showMoveGizmo;
    data.camera = persistence::ProjectCameraData::fromCamera(camera);
    data.editorSettings.showGrid = showGrid;
    data.editorSettings.gridSnapEnabled = m_gridSnapEnabled;
    data.editorSettings.placementMode = m_placementMode;
    data.editorSettings.allowRugOverlap = m_collisionOptions.allowRugOverlap;
    return data;
}

void RoomEditor::applyLoadedProjectData(const persistence::ProjectData& data, graphics::Camera& camera)
{
    m_interactionState.resetToIdle();
    m_inspectorTransformEditActive = false;
    m_commandHistory.clear();
    m_roomDraft = m_scene.getRoom().getSettings();
    m_lighting = data.lighting;
    m_lightPreset = data.lightingPreset;
    m_sampleIndex = std::clamp(data.renderSettings.msaaSampleIndex, 0, 2);
    wireframeMode = data.renderSettings.wireframe;
    showAxes = data.renderSettings.showAxes;
    showSelectedBBox = data.renderSettings.showSelectedBBox;
    showMoveGizmo = data.renderSettings.showMoveGizmo;
    showGrid = data.editorSettings.showGrid;
    m_gridSnapEnabled = data.editorSettings.gridSnapEnabled;
    m_placementMode = data.editorSettings.placementMode;
    m_collisionOptions.allowRugOverlap = data.editorSettings.allowRugOverlap;

    m_selection.clearSelection();
    if (!m_scene.getObjects().empty()) {
        m_selection.selectObject(m_scene.getObjects().front().id);
    }
    m_renameSyncedObjectId = scene::kInvalidObjectId;
    m_interactionState.warningMessage.clear();
    m_cameraController.syncFromCamera(camera);
    m_collisionDirty = true;
}

void RoomEditor::applyPresetImmediate(scene::ScenePresetType preset,
                                      assets::AssetManager& assetManager,
                                      graphics::Camera& camera)
{
    m_interactionState.resetToIdle();
    m_inspectorTransformEditActive = false;
    m_commandHistory.clear();

    std::string err;
    if (!scene::ScenePresets::applyPreset(preset, m_scene, assetManager.getCatalog(), &err)) {
        m_notifications.push(NotificationLevel::Error, "Preset failed: " + err, 4.5f);
        return;
    }

    m_activePreset = preset;
    m_isCustomEmptyRoom = false;
    std::string presetName = "Room Design";
    if (preset == scene::ScenePresetType::ModernBedroom) presetName = "Modern Bedroom";
    else if (preset == scene::ScenePresetType::GamingWorkspace) presetName = "Gaming Workspace";
    else if (preset == scene::ScenePresetType::MinimalistLivingRoom) presetName = "Minimalist Living Room";
    m_projectManager.resetForNewProject(presetName, false);

    m_roomDraft = m_scene.getRoom().getSettings();
    m_selection.clearSelection();
    if (!m_scene.getObjects().empty()) {
        m_selection.selectObject(m_scene.getObjects().front().id);
    }
    m_renameSyncedObjectId = scene::kInvalidObjectId;
    m_interactionState.warningMessage.clear();
    m_collisionDirty = true;
    refreshCollisionReport(assetManager);
    m_statusMessage = "Scene ready";
    m_notifications.push(NotificationLevel::Success, "Loaded " + presetName);

    setPerspective3DCamera(camera);
}

void RoomEditor::createNewEmptyRoomImmediate(graphics::Camera& camera)
{
    m_interactionState.resetToIdle();
    m_inspectorTransformEditActive = false;
    m_commandHistory.clear();
    m_scene.clearObjects();
    m_scene.getRoom().resetToDefault();
    m_isCustomEmptyRoom = true;
    m_projectManager.resetForNewProject("Untitled Room", false);
    m_roomDraft = m_scene.getRoom().getSettings();
    m_selection.clearSelection();
    m_renameSyncedObjectId = scene::kInvalidObjectId;
    m_interactionState.warningMessage.clear();
    m_collisionReport = systems::SceneCollisionReport{};
    m_collisionDirty = false;
    m_statusMessage = "New empty room ready";
    m_notifications.push(NotificationLevel::Info, "Created new project (5.0 x 6.0 x 3.0 m)");
    setPerspective3DCamera(camera);
}

void RoomEditor::requestNewEmptyRoom(graphics::Camera& camera)
{
    m_interactionState.resetToIdle();
    if (m_projectManager.isDirty()) {
        m_pendingDirtyAction = PendingDirtyAction::NewProject;
        m_openDirtyActionModal = true;
    } else {
        createNewEmptyRoomImmediate(camera);
    }
}

void RoomEditor::requestPresetChange(scene::ScenePresetType preset,
                                     assets::AssetManager& assetManager,
                                     graphics::Camera& camera)
{
    m_interactionState.resetToIdle();
    if (m_projectManager.isDirty()) {
        m_pendingPreset = preset;
        m_pendingDirtyAction = PendingDirtyAction::ApplyPreset;
        m_openDirtyActionModal = true;
    } else {
        applyPresetImmediate(preset, assetManager, camera);
    }
}

void RoomEditor::requestOpenProjectDialog(assets::AssetManager& assetManager, graphics::Camera& camera)
{
    m_interactionState.resetToIdle();
    if (m_projectManager.isDirty()) {
        m_pendingDirtyAction = PendingDirtyAction::OpenDialog;
        m_openDirtyActionModal = true;
        return;
    }
    std::filesystem::path chosen;
    const auto initDir = m_projectManager.hasCurrentFilePath()
                             ? m_projectManager.getCurrentFilePath().parent_path()
                             : std::filesystem::path{};
    if (core::FileDialog::openProjectFile(chosen, initDir)) {
        openProjectImmediate(chosen, assetManager, camera);
    }
}

void RoomEditor::requestOpenProjectPath(const std::filesystem::path& path,
                                        assets::AssetManager& assetManager,
                                        graphics::Camera& camera)
{
    m_interactionState.resetToIdle();
    if (m_projectManager.isDirty()) {
        m_pendingOpenPath = path;
        m_pendingDirtyAction = PendingDirtyAction::OpenSpecificPath;
        m_openDirtyActionModal = true;
        return;
    }
    openProjectImmediate(path, assetManager, camera);
}

bool RoomEditor::saveProjectToPathImmediate(const std::filesystem::path& path,
                                            assets::AssetManager& assetManager,
                                            const graphics::Camera& camera,
                                            std::string* outError)
{
    persistence::ProjectData data = buildProjectData(camera);
    data.projectName = persistence::ProjectManager::deriveProjectNameFromPath(path);
    std::string err;
    if (!m_projectManager.saveCurrentOrAs(path, data, assetManager.getCatalog(), &err)) {
        if (outError != nullptr) *outError = err;
        m_statusMessage = "Save failed";
        m_notifications.push(NotificationLevel::Error, "Save failed: " + err, 5.0f);
        return false;
    }
    m_statusMessage = "Saved '" + data.projectName + "'";
    m_notifications.push(NotificationLevel::Success, "Saved project '" + data.projectName + "'");
    return true;
}

bool RoomEditor::saveProject(assets::AssetManager& assetManager, const graphics::Camera& camera)
{
    if (m_projectManager.hasCurrentFilePath()) {
        return saveProjectToPathImmediate(m_projectManager.getCurrentFilePath(), assetManager, camera);
    }
    return saveProjectAs(assetManager, camera);
}

bool RoomEditor::saveProjectAs(assets::AssetManager& assetManager, const graphics::Camera& camera)
{
    std::filesystem::path chosen;
    const std::string defaultFile = getActiveSceneDisplayName() + ".roomcraft";
    const auto initDir = m_projectManager.hasCurrentFilePath()
                             ? m_projectManager.getCurrentFilePath().parent_path()
                             : std::filesystem::path{};
    if (!core::FileDialog::saveProjectFile(chosen, defaultFile, initDir)) {
        return false;
    }
    return saveProjectToPathImmediate(chosen, assetManager, camera);
}

bool RoomEditor::openProjectImmediate(const std::filesystem::path& path,
                                      assets::AssetManager& assetManager,
                                      graphics::Camera& camera,
                                      std::string* outError)
{
    persistence::ProjectData loadedData;
    std::string err;
    if (!m_projectManager.openTransactional(path, assetManager.getCatalog(), m_scene, camera, loadedData, &err)) {
        if (outError != nullptr) *outError = err;
        m_statusMessage = "Open project failed";
        m_notifications.push(NotificationLevel::Error, "Open failed: " + err, 5.5f);
        return false;
    }

    applyLoadedProjectData(loadedData, camera);
    refreshCollisionReport(assetManager);
    m_statusMessage = "Opened '" + loadedData.projectName + "'";
    m_notifications.push(NotificationLevel::Success, "Opened project '" + loadedData.projectName + "'");
    return true;
}

bool RoomEditor::requestApplicationClose()
{
    if (!m_projectManager.isDirty()) {
        m_shouldExitApplication = true;
        return true;
    }
    m_pendingDirtyAction = PendingDirtyAction::ExitApp;
    m_openDirtyActionModal = true;
    return false;
}

void RoomEditor::executePendingDirtyAction(assets::AssetManager& assetManager, graphics::Camera& camera)
{
    const PendingDirtyAction action = m_pendingDirtyAction;
    m_pendingDirtyAction = PendingDirtyAction::None;

    if (action == PendingDirtyAction::NewProject) {
        createNewEmptyRoomImmediate(camera);
    } else if (action == PendingDirtyAction::ApplyPreset) {
        applyPresetImmediate(m_pendingPreset, assetManager, camera);
    } else if (action == PendingDirtyAction::OpenDialog) {
        std::filesystem::path chosen;
        const auto initDir = m_projectManager.hasCurrentFilePath()
                                 ? m_projectManager.getCurrentFilePath().parent_path()
                                 : std::filesystem::path{};
        if (core::FileDialog::openProjectFile(chosen, initDir)) {
            openProjectImmediate(chosen, assetManager, camera);
        }
    } else if (action == PendingDirtyAction::OpenSpecificPath) {
        if (!m_pendingOpenPath.empty()) {
            openProjectImmediate(m_pendingOpenPath, assetManager, camera);
        }
        m_pendingOpenPath.clear();
    } else if (action == PendingDirtyAction::ExitApp) {
        m_shouldExitApplication = true;
    }
}

bool RoomEditor::undo(assets::AssetManager& assetManager)
{
    m_interactionState.resetToIdle();
    scene::ObjectId selId = m_selection.getSelectedObjectId();
    std::string desc;
    std::string err;
    if (!m_commandHistory.undo(m_scene, selId, assetManager.getCatalog(), &desc, &err)) {
        if (!err.empty()) {
            m_notifications.push(NotificationLevel::Error, "Undo failed: " + err);
        }
        return false;
    }
    if (scene::isValidObjectId(selId)) {
        m_selection.selectObject(selId);
    } else {
        m_selection.clearSelection();
    }
    m_roomDraft = m_scene.getRoom().getSettings();
    m_renameSyncedObjectId = scene::kInvalidObjectId;
    markSceneModified(true);
    refreshCollisionReport(assetManager);
    m_statusMessage = "Undid: " + desc;
    m_notifications.push(NotificationLevel::Info, "Undo: " + desc, 2.2f);
    return true;
}

bool RoomEditor::redo(assets::AssetManager& assetManager)
{
    m_interactionState.resetToIdle();
    scene::ObjectId selId = m_selection.getSelectedObjectId();
    std::string desc;
    std::string err;
    if (!m_commandHistory.redo(m_scene, selId, assetManager.getCatalog(), &desc, &err)) {
        if (!err.empty()) {
            m_notifications.push(NotificationLevel::Error, "Redo failed: " + err);
        }
        return false;
    }
    if (scene::isValidObjectId(selId)) {
        m_selection.selectObject(selId);
    } else {
        m_selection.clearSelection();
    }
    m_roomDraft = m_scene.getRoom().getSettings();
    m_renameSyncedObjectId = scene::kInvalidObjectId;
    markSceneModified(true);
    refreshCollisionReport(assetManager);
    m_statusMessage = "Redid: " + desc;
    m_notifications.push(NotificationLevel::Info, "Redo: " + desc, 2.2f);
    return true;
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
    recordUndoBeforeEdit("Duplicate object");
    std::string err;
    const auto newId = m_scene.duplicateObject(m_selection.getSelectedObjectId(), glm::vec3(0.45f, 0.0f, 0.45f), &err);
    if (scene::isValidObjectId(newId)) {
        m_selection.selectObject(newId);
        markSceneModified(true);
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
    recordUndoBeforeEdit("Reset transform");
    scene::Transform resetTr;
    resetTr.position = glm::vec3(0.0f, (obj->assetId == "blender-carpet") ? 0.003f : 0.0f, 0.0f);
    if (m_scene.setObjectTransform(obj->id, resetTr)) {
        markSceneModified(true);
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
    recordUndoBeforeEdit("Change wall visibility");
    m_roomDraft.wallVisibilityMode = mode;
    std::string err;
    if (m_scene.getRoom().applySettings(m_roomDraft, &err)) {
        markSceneModified(false);
        m_statusMessage = std::string("Walls: ") + scene::wallVisibilityModeToString(mode);
    } else {
        m_notifications.push(NotificationLevel::Error, err);
    }
}

void RoomEditor::toggleCeilingVisibility()
{
    recordUndoBeforeEdit("Toggle ceiling visibility");
    m_roomDraft.wallVisibilityMode = scene::WallVisibilityMode::Manual;
    m_roomDraft.showCeiling = !m_scene.getRoom().getSettings().isCeilingVisible();
    std::string err;
    if (m_scene.getRoom().applySettings(m_roomDraft, &err)) {
        markSceneModified(false);
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

    const auto* entry = assetManager.getCatalog().findById(assetId);
    if (entry == nullptr) {
        m_notifications.push(NotificationLevel::Error, "Unknown asset: " + assetId, 4.0f);
        return;
    }

    // Nếu đang ở chế độ PreventInvalid, kiểm tra vị trí spawn trước khi thêm vào Scene
    scene::Transform spawnTr;
    spawnTr.position = systems::PlacementSystem::computeSpawnPosition(m_scene, *entry, m_gridSnapEnabled);
    if (m_placementMode == systems::PlacementValidationMode::PreventInvalid) {
        const auto check = systems::CollisionSystem::validateCandidateObject(
            m_scene, scene::kInvalidObjectId, assetId, spawnTr, makeBoundsLookup(assetManager), m_collisionOptions);
        if (check.hasAnyIssue()) {
            const std::string msg = check.hasBoundaryViolation()
                                        ? "Placement blocked: " + check.boundary.formatSummary()
                                        : "Placement blocked: overlaps with " + check.collidingObjectNames.front();
            m_notifications.push(NotificationLevel::Warning, msg, 4.0f);
            m_statusMessage = msg;
            return;
        }
    }

    recordUndoBeforeEdit("Add " + gpuAsset->cpuData.displayName);
    const auto newId = m_scene.addObject(assetId, "", spawnTr, assetManager.getCatalog(), &err);
    if (scene::isValidObjectId(newId)) {
        m_selection.selectObject(newId);
        markSceneModified(true);
        refreshCollisionReport(assetManager);
        m_rightBottomTab = 0;
        const std::string display = gpuAsset->cpuData.displayName;
        m_statusMessage = "Added '" + display + "'";
        m_notifications.push(NotificationLevel::Success, "Added '" + display + "' (#" + std::to_string(newId) + ")");
        if (m_placementMode == systems::PlacementValidationMode::WarningOnly) {
            if (const auto* st = m_collisionReport.findStatus(newId); st != nullptr && st->hasAnyIssue()) {
                m_notifications.push(NotificationLevel::Warning,
                                     st->hasCollision() ? ("Warning: overlaps with " + st->collidingObjectNames.front())
                                                        : st->boundary.formatSummary(),
                                     3.5f);
            }
        }
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
    opts.lighting = m_lighting;

    // Truyền danh sách vật thể đang vi phạm va chạm / biên phòng sang SceneRenderer để tô khung cảnh báo
    if (m_placementMode != systems::PlacementValidationMode::Off) {
        opts.outOfBoundsObjectIds = m_collisionReport.outOfBoundsObjectIds;
        for (const auto& kv : m_collisionReport.objectStatuses) {
            if (kv.second.hasCollision()) {
                opts.collidingObjectIds.push_back(kv.first);
            }
        }
    }

    m_activeSamples = framebuffer.bindMultisampled(1 << m_sampleIndex);
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
    framebuffer.resolveMultisampled();
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

    // Phím tắt dự án & Undo/Redo toàn cục (khi không gõ text / không mở popup)
    if (!ImGui::GetIO().WantTextInput && !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId)) {
        const bool ctrl = input.isCtrlDown();
        const bool shift = input.isShiftDown();
        if (input.isKeyPressed(0x0E) || (ctrl && input.isKeyPressed('n'))) {
            requestNewEmptyRoom(camera);
        } else if (input.isKeyPressed(0x0F) || (ctrl && input.isKeyPressed('o'))) {
            requestOpenProjectDialog(assetManager, camera);
        } else if (input.isKeyPressed(0x13) || (ctrl && input.isKeyPressed('s'))) {
            if (shift) {
                saveProjectAs(assetManager, camera);
            } else {
                saveProject(assetManager, camera);
            }
        } else if (input.isKeyPressed(0x1A) || (ctrl && input.isKeyPressed('z'))) {
            if (canUndo()) undo(assetManager);
        } else if (input.isKeyPressed(0x19) || (ctrl && input.isKeyPressed('y'))) {
            if (canRedo()) redo(assetManager);
        }
    }

    // Lưu snapshot trước ViewportInteraction để ghi Undo 1 lần cho Delete/Ctrl+D bằng phím tắt
    const auto preInputSnapshot = systems::CommandHistory::capture(m_scene, m_selection.getSelectedObjectId(), "Edit scene");

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

    if (outcome.objectDeleted) {
        auto snap = preInputSnapshot;
        snap.description = "Delete object";
        m_commandHistory.pushUndo(std::move(snap));
    } else if (outcome.objectDuplicated) {
        auto snap = preInputSnapshot;
        snap.description = "Duplicate object";
        m_commandHistory.pushUndo(std::move(snap));
    }

    if (outcome.transformConfirmed && scene::isValidObjectId(outcome.transformedObjectId)) {
        const auto* movedObj = m_scene.findObject(outcome.transformedObjectId);
        if (movedObj != nullptr) {
            const bool actuallyChanged =
                glm::length(movedObj->transform.position - outcome.transformBefore.position) > 1e-5f ||
                glm::length(movedObj->transform.rotationDegrees - outcome.transformBefore.rotationDegrees) > 1e-4f ||
                glm::length(movedObj->transform.scale - outcome.transformBefore.scale) > 1e-5f;

            if (actuallyChanged) {
                bool blockedByPrevent = false;
                if (m_placementMode == systems::PlacementValidationMode::PreventInvalid) {
                    const auto st = systems::CollisionSystem::validateCandidateObject(
                        m_scene, movedObj->id, movedObj->assetId, movedObj->transform,
                        makeBoundsLookup(assetManager), m_collisionOptions);
                    if (st.hasAnyIssue()) {
                        // Khôi phục nguyên trạng transform hợp lệ trước kéo mà không mở hộp thoại chặn chuột
                        m_scene.setObjectTransform(movedObj->id, outcome.transformBefore);
                        blockedByPrevent = true;
                        const std::string msg = st.hasBoundaryViolation()
                                                    ? ("Placement prevented: " + st.boundary.formatSummary())
                                                    : ("Placement prevented: overlaps with " + st.collidingObjectNames.front());
                        m_statusMessage = msg;
                        m_notifications.push(NotificationLevel::Warning, msg, 3.8f);
                    }
                }
                if (!blockedByPrevent) {
                    // Ghi ĐÚNG 1 mục Undo cho toàn bộ thao tác kéo hoàn tất (KHÔNG ghi mỗi frame di chuột)
                    auto snap = systems::CommandHistory::capture(m_scene, movedObj->id, outcome.transformDescription);
                    for (auto& o : snap.objects) {
                        if (o.id == movedObj->id) {
                            o.transform = outcome.transformBefore;
                            break;
                        }
                    }
                    m_commandHistory.pushUndo(std::move(snap));
                    markSceneModified(true);
                }
                m_collisionDirty = true;
            }
        }
    } else if (outcome.sceneModified) {
        markSceneModified(true);
    }

    if (m_collisionDirty) {
        refreshCollisionReport(assetManager);
    }

    if (!outcome.statusMessage.empty() && !outcome.transformConfirmed) {
        m_statusMessage = outcome.statusMessage;
    } else if (!outcome.statusMessage.empty() && m_placementMode != systems::PlacementValidationMode::PreventInvalid) {
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
                    recordUndoBeforeEdit(obj.visible ? "Hide object" : "Show object");
                    m_scene.setObjectVisibility(obj.id, !obj.visible);
                    markSceneModified(true);
                }
                if (ImGui::MenuItem(obj.locked ? "Unlock" : "Lock")) {
                    if (auto* mutObj = m_scene.findObject(obj.id)) {
                        recordUndoBeforeEdit(mutObj->locked ? "Unlock object" : "Lock object");
                        mutObj->locked = !mutObj->locked;
                        markSceneModified(false);
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
                recordUndoBeforeEdit(obj.visible ? "Hide object" : "Show object");
                m_scene.setObjectVisibility(obj.id, !obj.visible);
                markSceneModified(true);
            }
            ImGui::SameLine(0.0f, 4.0f);
            if (EditorIcons::smallIconButton("lock",
                                             obj.locked ? EditorIcon::LockClosed : EditorIcon::LockOpen,
                                             obj.locked,
                                             obj.locked ? "Unlock object" : "Lock object",
                                             obj.locked ? tokens::kWarning : tokens::kTextSecondary)) {
                if (auto* mutObj = m_scene.findObject(obj.id)) {
                    recordUndoBeforeEdit(mutObj->locked ? "Unlock object" : "Lock object");
                    mutObj->locked = !mutObj->locked;
                    markSceneModified(false);
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
                        recordUndoBeforeEdit("Rename object");
                        std::string err;
                        if (m_scene.renameObject(obj->id, m_renameBuf.data(), &err)) {
                            markSceneModified(false);
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
                        recordUndoBeforeEdit(vis ? "Show object" : "Hide object");
                        m_scene.setObjectVisibility(obj->id, vis);
                        markSceneModified(true);
                    }
                    ImGui::SameLine(0.0f, 16.0f);
                    if (ImGui::Checkbox("Locked", &obj->locked)) {
                        recordUndoBeforeEdit(obj->locked ? "Lock object" : "Unlock object");
                        markSceneModified(false);
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
                    bool rotDeactivated = false;
                    bool sclDeactivated = false;

                    const float posSpeed = m_gridSnapEnabled ? 0.10f : 0.05f;
                    changed |= renderAxisDragFloat3("Position (m)", edited.position, posSpeed, -30.0f, 30.0f, "%.2f", &posDeactivated);
                    if (m_gridSnapEnabled && posDeactivated) {
                        edited.position = systems::PlacementSystem::snapToGrid(
                            edited.position, m_scene.getRoom().getSettings().gridSpacing, false);
                        changed = true;
                    }

                    changed |= renderAxisDragFloat3("Rotation (deg)", edited.rotationDegrees, 1.0f, -360.0f, 360.0f, "%.1f", &rotDeactivated);

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
                        if (ImGui::IsItemDeactivatedAfterEdit()) {
                            sclDeactivated = true;
                        }
                    } else {
                        changed |= renderAxisDragFloat3("", edited.scale, 0.02f, 0.10f, 10.0f, "%.2f", &sclDeactivated);
                    }

                    if (changed) {
                        if (!m_inspectorTransformEditActive || m_inspectorLastValidObjectId != obj->id) {
                            m_inspectorBeforeEdit = systems::CommandHistory::capture(m_scene, obj->id, "Edit transform");
                            m_inspectorLastValidTransform = obj->transform;
                            m_inspectorLastValidObjectId = obj->id;
                            m_inspectorTransformEditActive = true;
                        }
                        std::string err;
                        if (m_scene.setObjectTransform(obj->id, edited, &err)) {
                            markSceneModified(true);
                        } else {
                            m_notifications.push(NotificationLevel::Error, err);
                        }
                    }

                    if (m_inspectorTransformEditActive && (posDeactivated || rotDeactivated || sclDeactivated)) {
                        bool reverted = false;
                        if (m_placementMode == systems::PlacementValidationMode::PreventInvalid) {
                            const auto st = systems::CollisionSystem::validateCandidateObject(
                                m_scene, obj->id, obj->assetId, obj->transform,
                                makeBoundsLookup(assetManager), m_collisionOptions);
                            if (st.hasAnyIssue()) {
                                m_scene.setObjectTransform(obj->id, m_inspectorLastValidTransform);
                                m_collisionDirty = true;
                                reverted = true;
                                const std::string msg = st.hasBoundaryViolation()
                                                            ? ("Edit prevented: " + st.boundary.formatSummary())
                                                            : ("Edit prevented: overlaps with " + st.collidingObjectNames.front());
                                m_notifications.push(NotificationLevel::Warning, msg, 3.8f);
                            }
                        }
                        if (!reverted) {
                            m_commandHistory.pushUndo(m_inspectorBeforeEdit);
                        }
                        m_inspectorTransformEditActive = false;
                    }

                    ImGui::Spacing();
                    if (ImGui::Button("Reset Transform", ImVec2(-1.0f, 26.0f))) {
                        resetSelectedTransform();
                    }
                    if (obj->locked) ImGui::EndDisabled();
                }

                // 3. Nhóm Dimensions & Placement Validation (M8)
                if (gpuAsset && ImGui::CollapsingHeader("Dimensions & Placement", ImGuiTreeNodeFlags_DefaultOpen)) {
                    const glm::vec3 localSz = gpuAsset->cpuData.localBounds.getSize();
                    const math::BoundingBox worldBox = obj->getWorldBounds(gpuAsset->cpuData.localBounds);
                    const glm::vec3 worldSz = worldBox.getSize();
                    ImGui::Text("Asset : %.2f x %.2f x %.2f m", localSz.x, localSz.y, localSz.z);
                    ImGui::Text("World : %.2f x %.2f x %.2f m", worldSz.x, worldSz.y, worldSz.z);

                    if (m_placementMode == systems::PlacementValidationMode::Off) {
                        ImGui::TextDisabled("Collision check: Off");
                    } else if (!obj->visible) {
                        ImGui::TextDisabled("Hidden object (excluded from collision check)");
                    } else if (const auto* st = m_collisionReport.findStatus(obj->id)) {
                        if (!st->hasAnyIssue()) {
                            ImGui::TextColored(tokens::kSuccess, "Placement valid");
                        } else {
                            if (st->hasCollision()) {
                                std::string names;
                                for (std::size_t i = 0; i < st->collidingObjectNames.size(); ++i) {
                                    if (i > 0) names += ", ";
                                    names += st->collidingObjectNames[i];
                                }
                                ImGui::PushStyleColor(ImGuiCol_Text, tokens::kWarning);
                                ImGui::TextWrapped("[Collision] Overlaps with: %s", names.c_str());
                                ImGui::PopStyleColor();
                            }
                            if (st->hasBoundaryViolation()) {
                                ImGui::PushStyleColor(ImGuiCol_Text, tokens::kError);
                                ImGui::TextWrapped("[Boundary] %s", st->boundary.formatSummary().c_str());
                                ImGui::PopStyleColor();
                            }
                        }
                    }
                }

                // 4. Vật liệu dùng chung, hệ số độ bóng riêng của instance.
                if (gpuAsset && ImGui::CollapsingHeader("Material")) {
                    ImGui::Text("Materials: %zu | Textures: %zu",
                                gpuAsset->cpuData.materials.size(),
                                gpuAsset->cpuData.getUniqueTextureCount());
                    ImGui::BeginDisabled(obj->locked);
                    if (ImGui::SliderFloat("Shininess scale", &obj->shininessScale, 0.25f, 4.0f, "%.2fx",
                                           ImGuiSliderFlags_AlwaysClamp)) {
                        markSceneModified(false);
                    }
                    ImGui::EndDisabled();
                    for (std::size_t i = 0; i < gpuAsset->cpuData.materials.size(); ++i) {
                        const auto& mat = gpuAsset->cpuData.materials[i];
                        const auto* texture = gpuAsset->materialDiffuseTextures[i].get();
                        ImGui::TextWrapped("%s", mat.name.c_str());
                        ImGui::TextDisabled("Diffuse: %s", !texture ? "Base color" :
                                            texture->isFallback() ? "Fallback checker" : "Texture");
                        ImGui::TextDisabled("Ks %.2f %.2f %.2f | Ns %.1f",
                                            mat.specularColor.r, mat.specularColor.g, mat.specularColor.b, mat.shininess * obj->shininessScale);
                    }
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

void RoomEditor::renderRoomSettingsContent(assets::AssetManager& assetManager)
{
    if (ImGui::CollapsingHeader("Collision & Placement", ImGuiTreeNodeFlags_DefaultOpen)) {
        static constexpr const char* kModeLabels[] = {"Off", "Warning Only", "Prevent Invalid"};
        int modeIdx = static_cast<int>(m_placementMode);
        if (ImGui::Combo("Validation Mode", &modeIdx, kModeLabels, 3)) {
            setPlacementValidationMode(static_cast<systems::PlacementValidationMode>(modeIdx));
        }
        if (ImGui::Checkbox("Allow furniture on rugs", &m_collisionOptions.allowRugOverlap)) {
            markSceneModified(true);
        }
        if (m_placementMode != systems::PlacementValidationMode::Off) {
            ImGui::TextDisabled("Active: %zu | Overlaps: %zu | Out of bounds: %zu (%.2f ms)",
                                m_collisionReport.activeObjects,
                                m_collisionReport.collisions.size(),
                                m_collisionReport.outOfBoundsObjectIds.size(),
                                m_collisionReport.elapsedMs);
        }
    }
    if (ImGui::CollapsingHeader("Lighting Settings")) {
        const char* presets[] = {"Daylight", "Warm Interior", "Neutral Studio", "Evening"};
        if (ImGui::BeginCombo("Preset", m_lightPreset < 0 ? "Custom" : presets[m_lightPreset])) {
            for (int i = 0; i < 4; ++i) {
                if (ImGui::Selectable(presets[i], i == m_lightPreset)) {
                    m_lightPreset = i;
                    m_lighting = graphics::LightingSettings::preset(static_cast<graphics::LightingPreset>(i));
                    markSceneModified(false);
                }
            }
            ImGui::EndCombo();
        }
        bool edited = ImGui::SliderFloat("Ambient", &m_lighting.ambient, 0, 1);
        edited |= ImGui::Checkbox("Directional light", &m_lighting.sunEnabled);
        edited |= ImGui::SliderFloat3("Direction", &m_lighting.direction.x, -1, 1);
        edited |= ImGui::SliderFloat("Sun intensity", &m_lighting.sunIntensity, 0, 3);
        edited |= ImGui::ColorEdit3("Sun color", &m_lighting.sunColor.x);
        edited |= ImGui::Checkbox("Point light", &m_lighting.pointEnabled);
        edited |= ImGui::DragFloat3("Position (m)", &m_lighting.position.x, 0.05f);
        edited |= ImGui::SliderFloat("Point intensity", &m_lighting.pointIntensity, 0, 5);
        edited |= ImGui::ColorEdit3("Point color", &m_lighting.pointColor.x);
        edited |= ImGui::SliderFloat("Constant", &m_lighting.attenuation.x, 0.1f, 3);
        edited |= ImGui::SliderFloat("Linear", &m_lighting.attenuation.y, 0, 2);
        edited |= ImGui::SliderFloat("Quadratic", &m_lighting.attenuation.z, 0, 2);
        if (edited) {
            m_lightPreset = -1;
            markSceneModified(false);
        }
        if (ImGui::Button("Reset lighting")) {
            m_lightPreset = 0;
            m_lighting = {};
            markSceneModified(false);
        }
        const char* samples[] = {"Off", "2x", "4x"};
        if (ImGui::Combo("MSAA", &m_sampleIndex, samples, 3)) {
            markSceneModified(false);
        }
        ImGui::TextDisabled("Active samples: %d", m_activeSamples);
    }
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
        recordUndoBeforeEdit("Resize / Update room");
        std::string err;
        if (m_scene.getRoom().applySettings(m_roomDraft, &err)) {
            markSceneModified(true);
            refreshCollisionReport(assetManager);
            if (!m_collisionReport.outOfBoundsObjectIds.empty()) {
                m_notifications.push(
                    NotificationLevel::Warning,
                    "Room resized: " + std::to_string(m_collisionReport.outOfBoundsObjectIds.size()) +
                        " object(s) now exceed room bounds (kept without auto-delete).",
                    4.5f);
            } else {
                m_notifications.push(NotificationLevel::Success, "Room settings applied.");
            }
        } else {
            m_notifications.push(NotificationLevel::Error, err, 4.0f);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset", ImVec2(76.0f, 26.0f))) {
        recordUndoBeforeEdit("Reset room settings");
        m_roomDraft = scene::RoomSettings{};
        std::string err;
        m_scene.getRoom().applySettings(m_roomDraft, &err);
        markSceneModified(true);
        refreshCollisionReport(assetManager);
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

    // Bên phải: Cảnh báo va chạm/biên phòng (nếu có) + chấm trạng thái + status message ngắn gọn
    std::string rightText = m_interactionState.isIdle()
                                ? m_statusMessage
                                : std::string(systems::interactionPhaseToString(m_interactionState.phase));
    const bool hasCollisionWarnings = (m_placementMode != systems::PlacementValidationMode::Off) &&
                                      m_collisionReport.hasAnyIssues();
    if (hasCollisionWarnings && m_interactionState.isIdle()) {
        rightText = "Warnings: " + std::to_string(m_collisionReport.collisions.size()) + " overlap, " +
                    std::to_string(m_collisionReport.outOfBoundsObjectIds.size()) + " bounds";
    }
    const ImVec2 rightSz = ImGui::CalcTextSize(rightText.c_str());
    const float rightX = rect.width - rightSz.x - 16.0f;
    if (rightX > ImGui::GetCursorPosX() + 20.0f) {
        ImGui::SameLine(rightX);
        const ImVec2 dotPos(rect.x + rightX - 10.0f, rect.y + rect.height * 0.5f);
        const ImVec4 dotCol = (!m_interactionState.warningMessage.empty() || hasCollisionWarnings)
                                  ? tokens::kWarning
                                  : tokens::kSuccess;
        ImGui::GetWindowDrawList()->AddCircleFilled(dotPos, 4.0f, EditorTheme::toU32(dotCol));
        ImGui::TextUnformatted(rightText.c_str());
    }

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

void RoomEditor::renderConfirmationModals(assets::AssetManager& assetManager, graphics::Camera& camera)
{
    if (m_openDirtyActionModal) {
        ImGui::OpenPopup("Unsaved Changes");
        m_openDirtyActionModal = false;
    }
    if (ImGui::BeginPopupModal("Unsaved Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("Project '%s' has unsaved changes. Would you like to save before continuing?",
                           getActiveSceneDisplayName().c_str());
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, tokens::kPrimary);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.06f, 0.08f, 0.12f, 1.0f));
        if (ImGui::Button("Save", ImVec2(96.0f, 0.0f))) {
            // Chỉ thực thi hành động chờ nếu lưu thành công; nếu người dùng hủy hoặc lỗi ghi -> giữ nguyên dự án!
            if (saveProject(assetManager, camera)) {
                ImGui::CloseCurrentPopup();
                executePendingDirtyAction(assetManager, camera);
            } else {
                m_pendingDirtyAction = PendingDirtyAction::None;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::PopStyleColor(2);
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.24f, 0.24f, 1.0f));
        if (ImGui::Button("Discard", ImVec2(96.0f, 0.0f))) {
            ImGui::CloseCurrentPopup();
            executePendingDirtyAction(assetManager, camera);
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(96.0f, 0.0f))) {
            m_pendingDirtyAction = PendingDirtyAction::None;
            m_pendingOpenPath.clear();
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
                recordUndoBeforeEdit("Delete " + obj->name);
                const std::string nameCopy = obj->name;
                m_scene.removeObject(m_pendingDeleteObjectId);
                m_interactionState.abortIfInvalid(m_scene);
                m_selection.reconcileWithScene(m_scene);
                markSceneModified(true);
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
