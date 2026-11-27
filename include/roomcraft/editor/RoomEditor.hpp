#pragma once

#include "roomcraft/assets/AssetManager.hpp"
#include "roomcraft/graphics/Camera.hpp"
#include "roomcraft/graphics/Framebuffer.hpp"
#include "roomcraft/graphics/Mesh.hpp"
#include "roomcraft/graphics/Renderer.hpp"
#include "roomcraft/graphics/SceneRenderer.hpp"
#include "roomcraft/graphics/Shader.hpp"
#include "roomcraft/scene/Scene.hpp"
#include "roomcraft/scene/ScenePresets.hpp"
#include "roomcraft/systems/SelectionState.hpp"

#include <glm/vec4.hpp>

#include <array>
#include <string>

namespace roomcraft::editor {

// Lớp RoomEditor đóng gói toàn bộ trạng thái và giao diện Dear ImGui cho chế độ M4 Room Editor
// (tách khỏi Application.cpp để giữ mã nguồn gọn gàng, dễ bảo trì).
class RoomEditor {
public:
    RoomEditor() = default;

    // Khởi tạo phòng mặc định (Modern Bedroom Preset) và căn camera Fit Room
    void initialize(assets::AssetManager& assetManager, graphics::Camera& camera);

    // Chuyển góc nhìn Camera theo căn phòng hoặc đối tượng đang chọn
    void fitCameraToRoom(graphics::Camera& camera) const;
    void fitCameraToSelected(assets::AssetManager& assetManager, graphics::Camera& camera) const;
    void setIsometricCamera(graphics::Camera& camera) const;
    void setTopViewCamera(graphics::Camera& camera) const;
    void setFrontViewCamera(graphics::Camera& camera) const;
    void setPerspective3DCamera(graphics::Camera& camera) const;

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

    // Render toàn bộ giao diện Dear ImGui của Room Editor (Toolbar, Library, Hierarchy, Viewport, Inspector, Room Settings, Status Bar)
    void renderUi(int windowWidth,
                  int windowHeight,
                  float topBarHeight,
                  assets::AssetManager& assetManager,
                  graphics::Camera& camera,
                  graphics::Framebuffer& framebuffer,
                  float fps,
                  float frameTimeMs);

    void shutdownGpuResources();

    scene::Scene& getScene() { return m_scene; }
    const scene::Scene& getScene() const { return m_scene; }
    systems::SelectionState& getSelection() { return m_selection; }
    const systems::SelectionState& getSelection() const { return m_selection; }
    graphics::SceneRenderer& getSceneRenderer() { return m_sceneRenderer; }

private:
    void applyPresetImmediate(scene::ScenePresetType preset,
                              assets::AssetManager& assetManager,
                              graphics::Camera& camera);
    void createNewEmptyRoom(graphics::Camera& camera);
    void requestPresetChange(scene::ScenePresetType preset,
                             assets::AssetManager& assetManager,
                             graphics::Camera& camera);

    void renderToolbar(int windowWidth,
                       float posY,
                       float height,
                       assets::AssetManager& assetManager,
                       graphics::Camera& camera);
    void renderLeftPanel(float posX,
                         float posY,
                         float width,
                         float height,
                         assets::AssetManager& assetManager);
    void renderViewportPanel(float posX,
                             float posY,
                             float width,
                             float height,
                             graphics::Framebuffer& framebuffer,
                             graphics::Camera& camera);
    void renderRightPanel(float posX,
                          float posY,
                          float width,
                          float height,
                          assets::AssetManager& assetManager,
                          graphics::Camera& camera);
    void renderStatusBar(int windowWidth,
                         float posY,
                         float height,
                         const assets::AssetManager& assetManager,
                         float fps,
                         float frameTimeMs);
    void renderConfirmPresetModal(assets::AssetManager& assetManager, graphics::Camera& camera);

private:
    scene::Scene m_scene;
    systems::SelectionState m_selection;
    graphics::SceneRenderer m_sceneRenderer;

    scene::RoomSettings m_roomDraft;
    scene::ScenePresetType m_activePreset = scene::ScenePresetType::ModernBedroom;
    scene::ScenePresetType m_pendingPreset = scene::ScenePresetType::ModernBedroom;
    bool m_pendingNewEmptyRoom = false;
    bool m_openConfirmModal = false;
    bool m_sceneDirty = false;

    // Tùy chọn hiển thị
    bool m_wireframe = false;
    bool m_depthTest = true;
    bool m_showGrid = true;
    bool m_showAxes = true;
    bool m_showSelectedBBox = true;
    bool m_gridSnapEnabled = false;
    bool m_uniformScale = false;

    // Bộ lọc Furniture Library & Hierarchy
    int m_librarySourceFilter = 0;   // 0=All, 1=Procedural, 2=Blender
    int m_libraryCategoryFilter = 0; // 0=All, 1..7
    std::array<char, 128> m_librarySearchBuf{};
    std::array<char, 128> m_hierarchySearchBuf{};

    // Đổi tên trong Inspector
    scene::ObjectId m_renameSyncedObjectId = scene::kInvalidObjectId;
    std::array<char, 128> m_renameBuf{};

    std::string m_statusMessage = "Room Editor ready.";
    std::string m_errorMessage;
};

} // namespace roomcraft::editor
