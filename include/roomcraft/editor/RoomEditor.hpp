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
#include "roomcraft/scene/Scene.hpp"
#include "roomcraft/scene/ScenePresets.hpp"
#include "roomcraft/systems/CameraController.hpp"
#include "roomcraft/systems/InteractionState.hpp"
#include "roomcraft/systems/SelectionState.hpp"

#include <glm/vec4.hpp>

#include <array>
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
    void setActiveTool(systems::TransformMode mode);
    void duplicateSelectedObject();
    void requestDeleteSelectedObject();
    void resetSelectedTransform();
    void focusRoomSettingsTab();
    void setWallVisibilityMode(scene::WallVisibilityMode mode);
    void toggleCeilingVisibility();
    void resetLayout();
    void setUiScale(float scale);

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

    scene::RoomSettings m_roomDraft;
    scene::ScenePresetType m_activePreset = scene::ScenePresetType::ModernBedroom;
    bool m_isCustomEmptyRoom = false;

    // Hộp thoại xác nhận
    scene::ScenePresetType m_pendingPreset = scene::ScenePresetType::ModernBedroom;
    bool m_pendingNewEmptyRoom = false;
    bool m_openConfirmSceneModal = false;
    bool m_openConfirmDeleteModal = false;
    scene::ObjectId m_pendingDeleteObjectId = scene::kInvalidObjectId;
    bool m_sceneDirty = false;

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
