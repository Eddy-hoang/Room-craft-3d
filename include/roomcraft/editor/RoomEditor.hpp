#pragma once

#include "roomcraft/assets/AssetManager.hpp"
#include "roomcraft/core/InputManager.hpp"
#include "roomcraft/editor/EditorLayout.hpp"
#include "roomcraft/editor/EditorTheme.hpp"
#include "roomcraft/editor/NotificationSystem.hpp"
#include "roomcraft/editor/ViewportInteraction.hpp"
#include "roomcraft/graphics/Camera.hpp"
#include "roomcraft/graphics/Framebuffer.hpp"
#include "roomcraft/graphics/Mesh.hpp"
#include "roomcraft/graphics/Renderer.hpp"
#include "roomcraft/graphics/SceneRenderer.hpp"
#include "roomcraft/graphics/Shader.hpp"
#include "roomcraft/graphics/ThumbnailRenderer.hpp"
#include "roomcraft/persistence/ProjectData.hpp"
#include "roomcraft/persistence/ProjectManager.hpp"
#include "roomcraft/scene/Scene.hpp"
#include "roomcraft/scene/ScenePresets.hpp"
#include "roomcraft/systems/CameraController.hpp"
#include "roomcraft/systems/CollisionSystem.hpp"
#include "roomcraft/systems/CommandHistory.hpp"
#include "roomcraft/systems/InteractionState.hpp"
#include "roomcraft/systems/SelectionState.hpp"

#include <glm/vec4.hpp>

#include <array>
#include <filesystem>
#include <string>

namespace roomcraft::editor {

// Lớp RoomEditor là không gian làm việc mặc định của RoomCraft 3D (M6 Professional CAD UI/UX).
// Kết hợp Furniture Library có thumbnail FBO thật, 3D Viewport + Move Gizmo, Scene Hierarchy có Context Menu,
// Properties Inspector phân nhóm, Resizable Splitters, Notifications và Confirmation Dialogs.
class RoomEditor {
public:
    RoomEditor() = default;

    void initialize(assets::AssetManager& assetManager, graphics::Camera& camera);

    // Điều khiển góc nhìn Camera (luôn đồng bộ CameraController)
    void fitCameraToRoom(graphics::Camera& camera);
    void fitCameraToSelected(assets::AssetManager& assetManager, graphics::Camera& camera);
    void setIsometricCamera(graphics::Camera& camera);
    void setTopViewCamera(graphics::Camera& camera);
    void setFrontViewCamera(graphics::Camera& camera);
    void setPerspective3DCamera(graphics::Camera& camera);

    // Các hành động gọi được trực tiếp từ Main Menu Bar (File / Edit / View / Room)
    void requestNewEmptyRoom(graphics::Camera& camera);
    void requestPresetChange(scene::ScenePresetType preset,
                             assets::AssetManager& assetManager,
                             graphics::Camera& camera);
    void requestOpenProjectDialog(assets::AssetManager& assetManager, graphics::Camera& camera);
    void requestOpenProjectPath(const std::filesystem::path& path,
                                assets::AssetManager& assetManager,
                                graphics::Camera& camera);
    bool saveProject(assets::AssetManager& assetManager, const graphics::Camera& camera);
    bool saveProjectAs(assets::AssetManager& assetManager, const graphics::Camera& camera);
    bool openProjectImmediate(const std::filesystem::path& path,
                              assets::AssetManager& assetManager,
                              graphics::Camera& camera,
                              std::string* outError = nullptr);
    bool saveProjectToPathImmediate(const std::filesystem::path& path,
                                    assets::AssetManager& assetManager,
                                    const graphics::Camera& camera,
                                    std::string* outError = nullptr);
    // Trả về true nếu an toàn đóng ngay; trả về false nếu đang dirty và đã mở hộp thoại Save/Discard/Cancel
    bool requestApplicationClose();
    bool shouldExitApplication() const { return m_shouldExitApplication; }

    // Undo / Redo (P2 - M8.10)
    bool undo(assets::AssetManager& assetManager);
    bool redo(assets::AssetManager& assetManager);
    bool canUndo() const { return m_commandHistory.canUndo(); }
    bool canRedo() const { return m_commandHistory.canRedo(); }

    void setActiveTool(systems::TransformMode mode);
    void duplicateSelectedObject();
    void requestDeleteSelectedObject();
    void resetSelectedTransform();
    void focusRoomSettingsTab();
    void setWallVisibilityMode(scene::WallVisibilityMode mode);
    void toggleCeilingVisibility();
    void resetLayout();
    void setUiScale(float scale);
    void setMsaaSampleIndex(int index) { m_sampleIndex = index; }
    int getActiveSamples() const { return m_activeSamples; }

    // Cấu hình & truy vấn Collision Detection (M8)
    void setPlacementValidationMode(systems::PlacementValidationMode mode);
    systems::PlacementValidationMode getPlacementValidationMode() const { return m_placementMode; }
    void setCollisionOptions(const systems::CollisionOptions& opts);
    const systems::CollisionOptions& getCollisionOptions() const { return m_collisionOptions; }
    const systems::SceneCollisionReport& getCollisionReport() const { return m_collisionReport; }
    void refreshCollisionReport(assets::AssetManager& assetManager);

    // Đóng gói & khôi phục dữ liệu dự án (M8.5 - M8.8)
    persistence::ProjectData buildProjectData(const graphics::Camera& camera) const;
    persistence::ProjectManager& getProjectManager() { return m_projectManager; }
    const persistence::ProjectManager& getProjectManager() const { return m_projectManager; }
    bool isDirty() const { return m_projectManager.isDirty(); }
    std::string getWindowTitleLabel() const;

    // Render toàn bộ scene vào Framebuffer hiện tại
    void renderSceneToFramebuffer(assets::AssetManager& assetManager,
                                  graphics::Camera& camera,
                                  graphics::Framebuffer& framebuffer,
                                  graphics::Renderer& renderer,
                                  graphics::Shader& meshShader,
                                  graphics::Shader& gridShader,
                                  const graphics::Mesh& axisXMesh,
                                  const graphics::Mesh& axisYMesh,
                                  const graphics::Mesh& axisZMesh,
                                  const glm::vec4& clearColor,
                                  float faceContrast);

    // Render giao diện Room Editor + định tuyến tương tác M5/M6 (trả về true nếu đã tiêu thụ Esc)
    bool renderUiAndProcessInput(int windowWidth,
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
                                 float frameTimeMs);

    void shutdownGpuResources();

    // Truy cập trạng thái phục vụ Main Menu, Diagnostics & Unit/Integration Tests
    scene::Scene& getScene() { return m_scene; }
    const scene::Scene& getScene() const { return m_scene; }
    systems::SelectionState& getSelection() { return m_selection; }
    const systems::SelectionState& getSelection() const { return m_selection; }
    systems::CameraController& getCameraController() { return m_cameraController; }
    const systems::CameraController& getCameraController() const { return m_cameraController; }
    systems::InteractionState& getInteractionState() { return m_interactionState; }
    const systems::InteractionState& getInteractionState() const { return m_interactionState; }
    EditorLayout& getLayout() { return m_layout; }
    const EditorLayout& getLayout() const { return m_layout; }
    NotificationSystem& getNotifications() { return m_notifications; }
    const ViewportInputContext& getViewportInputContext() const { return m_viewportInputCtx; }
    graphics::SceneRenderer& getSceneRenderer() { return m_sceneRenderer; }
    std::string getActiveSceneDisplayName() const;

    // Cờ hiển thị dùng chung với menu View
    bool showGrid = true;
    bool showAxes = true;
    bool wireframeMode = false;
    bool showSelectedBBox = true;
    bool showMoveGizmo = true;
    bool showPerformanceInStatus = false;

private:
    enum class PendingDirtyAction {
        None = 0,
        NewProject,
        ApplyPreset,
        OpenDialog,
        OpenSpecificPath,
        ExitApp
    };

    void markSceneModified(bool collisionAffected = true);
    void recordUndoBeforeEdit(const std::string& description);
    void executePendingDirtyAction(assets::AssetManager& assetManager, graphics::Camera& camera);
    void applyLoadedProjectData(const persistence::ProjectData& data, graphics::Camera& camera);
    systems::CollisionSystem::LocalBoundsLookupFn makeBoundsLookup(assets::AssetManager& assetManager) const;

    void applyPresetImmediate(scene::ScenePresetType preset,
                              assets::AssetManager& assetManager,
                              graphics::Camera& camera);
    void createNewEmptyRoomImmediate(graphics::Camera& camera);
    void addFurnitureToScene(const std::string& assetId, assets::AssetManager& assetManager);

    void renderToolbar(const PanelRect& rect,
                       assets::AssetManager& assetManager,
                       graphics::Camera& camera);
    void renderFurnitureLibraryPanel(const PanelRect& rect,
                                     assets::AssetManager& assetManager,
                                     graphics::ThumbnailRenderer& thumbnailRenderer,
                                     graphics::Shader& meshShader);
    void renderViewportPanel(const PanelRect& rect,
                             graphics::Framebuffer& framebuffer,
                             graphics::Camera& camera);
    void renderViewportOverlays(const PanelRect& rect, graphics::Camera& camera);
    void renderHierarchyPanel(const PanelRect& rect,
                              assets::AssetManager& assetManager,
                              graphics::Camera& camera);
    void renderPropertiesPanel(const PanelRect& rect,
                               assets::AssetManager& assetManager,
                               graphics::Camera& camera);
    void renderRoomSettingsContent(assets::AssetManager& assetManager);
    void renderStatusBar(const PanelRect& rect,
                         const graphics::Camera& camera,
                         float fps,
                         float frameTimeMs);
    void renderSplitters(const ComputedWorkspaceLayout& layout);
    void renderConfirmationModals(assets::AssetManager& assetManager, graphics::Camera& camera);

    static bool renderAxisDragFloat3(const char* label,
                                     glm::vec3& vec,
                                     float speed,
                                     float minVal,
                                     float maxVal,
                                     const char* fmt,
                                     bool* outDeactivatedAfterEdit = nullptr);

private:
    scene::Scene m_scene;
    systems::SelectionState m_selection;
    systems::CameraController m_cameraController;
    systems::InteractionState m_interactionState;
    EditorLayout m_layout;
    NotificationSystem m_notifications;
    ViewportInputContext m_viewportInputCtx;
    graphics::SceneRenderer m_sceneRenderer;

    graphics::LightingSettings m_lighting;
    int m_lightPreset = 0;
    int m_sampleIndex = 0;
    int m_activeSamples = 1;
    scene::RoomSettings m_roomDraft;
    scene::ScenePresetType m_activePreset = scene::ScenePresetType::ModernBedroom;
    bool m_isCustomEmptyRoom = false;

    // Trạng thái Dự án, Lưu/Mở & Lịch sử Undo/Redo (M8)
    persistence::ProjectManager m_projectManager;
    systems::CommandHistory m_commandHistory;
    systems::PlacementValidationMode m_placementMode = systems::PlacementValidationMode::WarningOnly;
    systems::CollisionOptions m_collisionOptions;
    systems::SceneCollisionReport m_collisionReport;
    bool m_collisionDirty = true;

    // Hộp thoại xác nhận (Save / Discard / Cancel cho mọi hành động phá hủy khi dirty)
    PendingDirtyAction m_pendingDirtyAction = PendingDirtyAction::None;
    scene::ScenePresetType m_pendingPreset = scene::ScenePresetType::ModernBedroom;
    std::filesystem::path m_pendingOpenPath;
    bool m_openDirtyActionModal = false;
    bool m_openConfirmDeleteModal = false;
    scene::ObjectId m_pendingDeleteObjectId = scene::kInvalidObjectId;
    bool m_shouldExitApplication = false;

    // Theo dõi sửa thuộc tính Inspector để gộp thành 1 mục Undo khi nhả chuột
    bool m_inspectorTransformEditActive = false;
    systems::SceneHistorySnapshot m_inspectorBeforeEdit;
    scene::Transform m_inspectorLastValidTransform;
    scene::ObjectId m_inspectorLastValidObjectId = scene::kInvalidObjectId;

    // Trạng thái Furniture Library & Inspector
    int m_librarySourceFilter = 0;   // 0=All, 1=Procedural, 2=Blender
    int m_libraryCategoryFilter = 0; // 0=All, 1..7
    bool m_libraryGridView = true;
    std::string m_selectedLibraryAssetId = "blender-modern-sofa";
    std::array<char, 128> m_librarySearchBuf{};
    std::array<char, 128> m_hierarchySearchBuf{};

    int m_rightBottomTab = 0; // 0=Object Properties, 1=Room Settings
    bool m_requestRoomTabFocus = false;
    bool m_gridSnapEnabled = false;
    bool m_uniformScale = false;
    bool m_depthTest = true;

    scene::ObjectId m_renameSyncedObjectId = scene::kInvalidObjectId;
    std::array<char, 128> m_renameBuf{};

    std::string m_statusMessage = "Scene ready";
};

} // namespace roomcraft::editor
