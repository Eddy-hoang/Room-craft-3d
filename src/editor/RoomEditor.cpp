#include "roomcraft/editor/RoomEditor.hpp"

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
}

void RoomEditor::applyPresetImmediate(scene::ScenePresetType preset,
                                      assets::AssetManager& assetManager,
                                      graphics::Camera& camera)
{
    std::string err;
    if (!scene::ScenePresets::applyPreset(preset, m_scene, assetManager.getCatalog(), &err)) {
        m_errorMessage = err;
        return;
    }

    m_activePreset = preset;
    m_roomDraft = m_scene.getRoom().getSettings();
    m_selection.clearSelection();
    if (!m_scene.getObjects().empty()) {
        m_selection.selectObject(m_scene.getObjects().front().id);
    }
    m_renameSyncedObjectId = scene::kInvalidObjectId;
    m_sceneDirty = false;
    m_errorMessage.clear();
    m_statusMessage = std::string("Loaded preset: ") + scene::scenePresetTypeToString(preset);

    setPerspective3DCamera(camera);
}

void RoomEditor::createNewEmptyRoom(graphics::Camera& camera)
{
    m_scene.clearObjects();
    m_scene.getRoom().resetToDefault();
    m_roomDraft = m_scene.getRoom().getSettings();
    m_selection.clearSelection();
    m_renameSyncedObjectId = scene::kInvalidObjectId;
    m_sceneDirty = false;
    m_errorMessage.clear();
    m_statusMessage = "Created default empty room (5.0 x 6.0 x 3.0 m).";
    setPerspective3DCamera(camera);
}

void RoomEditor::requestPresetChange(scene::ScenePresetType preset,
                                     assets::AssetManager& assetManager,
                                     graphics::Camera& camera)
{
    if (m_sceneDirty && m_scene.getObjectCount() > 0) {
        m_pendingPreset = preset;
        m_pendingNewEmptyRoom = false;
        m_openConfirmModal = true;
    } else {
        applyPresetImmediate(preset, assetManager, camera);
    }
}

void RoomEditor::fitCameraToRoom(graphics::Camera& camera) const
{
    const math::BoundingBox bounds = m_scene.getRoom().getExteriorBounds();
    camera.fitBoundingBox(bounds, 1.22f);
}

void RoomEditor::fitCameraToSelected(assets::AssetManager& assetManager, graphics::Camera& camera) const
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
        camera.fitBoundingBox(worldBox, 1.35f);
    }
}

void RoomEditor::setPerspective3DCamera(graphics::Camera& camera) const
{
    camera.setProjectionMode(graphics::ProjectionMode::Perspective);
    camera.lookAt(glm::vec3(5.5f, 4.8f, 6.2f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    fitCameraToRoom(camera);
}

void RoomEditor::setIsometricCamera(graphics::Camera& camera) const
{
    camera.setProjectionMode(graphics::ProjectionMode::Orthographic);
    camera.lookAt(glm::vec3(7.0f, 6.2f, 7.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    fitCameraToRoom(camera);
}

void RoomEditor::setTopViewCamera(graphics::Camera& camera) const
{
    camera.setProjectionMode(graphics::ProjectionMode::Orthographic);
    camera.lookAt(glm::vec3(0.0f, 12.0f, 0.001f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    fitCameraToRoom(camera);
}

void RoomEditor::setFrontViewCamera(graphics::Camera& camera) const
{
    camera.setProjectionMode(graphics::ProjectionMode::Orthographic);
    const float halfH = m_scene.getRoom().getSettings().height * 0.5f;
    camera.lookAt(glm::vec3(0.0f, halfH, 10.0f), glm::vec3(0.0f, halfH, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    fitCameraToRoom(camera);
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

    graphics::SceneRenderOptions opts;
    opts.wireframe = m_wireframe;
    opts.depthTest = m_depthTest;
    opts.showGrid = m_showGrid;
    opts.showAxes = m_showAxes;
    opts.showSelectedBoundingBox = m_showSelectedBBox;
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

void RoomEditor::renderUi(int windowWidth,
                          int windowHeight,
                          float topBarHeight,
                          assets::AssetManager& assetManager,
                          graphics::Camera& camera,
                          graphics::Framebuffer& framebuffer,
                          float fps,
                          float frameTimeMs)
{
    m_selection.reconcileWithScene(m_scene);

    constexpr float kToolbarH = 38.0f;
    constexpr float kStatusH = 28.0f;
    const float w = static_cast<float>(std::max(windowWidth, 640));
    const float h = static_cast<float>(std::max(windowHeight, 480));

    const float contentY = topBarHeight + kToolbarH;
    const float contentH = std::max(220.0f, h - contentY - kStatusH);

    const float leftW = std::clamp(w * 0.23f, 250.0f, 330.0f);
    const float rightW = std::clamp(w * 0.24f, 270.0f, 350.0f);
    const float centerW = std::max(260.0f, w - leftW - rightW);

    renderToolbar(windowWidth, topBarHeight, kToolbarH, assetManager, camera);
    renderLeftPanel(0.0f, contentY, leftW, contentH, assetManager);
    renderViewportPanel(leftW, contentY, centerW, contentH, framebuffer, camera);
    renderRightPanel(leftW + centerW, contentY, rightW, contentH, assetManager, camera);
    renderStatusBar(windowWidth, contentY + contentH, kStatusH, assetManager, fps, frameTimeMs);
    renderConfirmPresetModal(assetManager, camera);
}

void RoomEditor::renderToolbar(int windowWidth,
                               float posY,
                               float height,
                               assets::AssetManager& assetManager,
                               graphics::Camera& camera)
{
    ImGui::SetNextWindowPos(ImVec2(0.0f, posY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(windowWidth), height), ImGuiCond_Always);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoScrollbar;
    ImGui::Begin("##RoomEditorToolbar", nullptr, flags);

    if (ImGui::Button("New Room")) {
        if (m_sceneDirty && m_scene.getObjectCount() > 0) {
            m_pendingNewEmptyRoom = true;
            m_openConfirmModal = true;
        } else {
            createNewEmptyRoom(camera);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Bedroom")) {
        requestPresetChange(scene::ScenePresetType::ModernBedroom, assetManager, camera);
    }
    ImGui::SameLine();
    if (ImGui::Button("Gaming")) {
        requestPresetChange(scene::ScenePresetType::GamingWorkspace, assetManager, camera);
    }
    ImGui::SameLine();
    if (ImGui::Button("Living")) {
        requestPresetChange(scene::ScenePresetType::MinimalistLivingRoom, assetManager, camera);
    }

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    const bool hasSel = m_selection.hasSelection();
    if (!hasSel) ImGui::BeginDisabled();
    if (ImGui::Button("Duplicate")) {
        std::string err;
        const auto newId = m_scene.duplicateObject(m_selection.getSelectedObjectId(), glm::vec3(0.45f, 0.0f, 0.45f), &err);
        if (scene::isValidObjectId(newId)) {
            m_selection.selectObject(newId);
            m_sceneDirty = true;
            m_errorMessage.clear();
            m_statusMessage = "Duplicated object (new ID #" + std::to_string(newId) + ").";
        } else {
            m_errorMessage = err;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete")) {
        const auto delId = m_selection.getSelectedObjectId();
        if (m_scene.removeObject(delId)) {
            m_selection.clearSelection();
            m_sceneDirty = true;
            m_errorMessage.clear();
            m_statusMessage = "Deleted object ID #" + std::to_string(delId) + ".";
        }
    }
    if (!hasSel) ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    if (ImGui::Button("3D")) setPerspective3DCamera(camera);
    ImGui::SameLine();
    if (ImGui::Button("Iso")) setIsometricCamera(camera);
    ImGui::SameLine();
    if (ImGui::Button("Top 2D")) setTopViewCamera(camera);
    ImGui::SameLine();
    if (ImGui::Button("Front")) setFrontViewCamera(camera);
    ImGui::SameLine();
    if (ImGui::Button("Fit Room")) fitCameraToRoom(camera);
    ImGui::SameLine();
    if (ImGui::Button("Fit Sel")) fitCameraToSelected(assetManager, camera);

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    ImGui::Checkbox("Wire", &m_wireframe);
    ImGui::SameLine();
    ImGui::Checkbox("BBox", &m_showSelectedBBox);
    ImGui::SameLine();
    ImGui::Checkbox("Snap", &m_gridSnapEnabled);

    ImGui::End();
}

void RoomEditor::renderLeftPanel(float posX,
                                 float posY,
                                 float width,
                                 float height,
                                 assets::AssetManager& assetManager)
{
    ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("Library & Hierarchy", nullptr, flags);

    const float halfH = std::max(140.0f, (height - 80.0f) * 0.50f);

    // ==================== 1. FURNITURE LIBRARY ====================
    ImGui::TextColored(ImVec4(0.45f, 0.80f, 1.0f, 1.0f), "FURNITURE LIBRARY (14 Assets)");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##LibSearch", "Search furniture...", m_librarySearchBuf.data(), m_librarySearchBuf.size());

    static constexpr const char* kSourceItems[] = {"All", "Procedural", "Blender"};
    ImGui::SetNextItemWidth(width * 0.44f);
    ImGui::Combo("##LibSrc", &m_librarySourceFilter, kSourceItems, 3);
    ImGui::SameLine();
    static constexpr const char* kCatItems[] = {"All Cats", "Seating", "Tables", "Storage", "Lighting", "Beds", "Electronics", "Decor"};
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::Combo("##LibCat", &m_libraryCategoryFilter, kCatItems, 8);

    std::optional<assets::AssetSourceType> srcFilter;
    if (m_librarySourceFilter == 1) srcFilter = assets::AssetSourceType::Procedural;
    if (m_librarySourceFilter == 2) srcFilter = assets::AssetSourceType::Imported;

    std::optional<assets::AssetCategory> catFilter;
    if (m_libraryCategoryFilter > 0) {
        catFilter = static_cast<assets::AssetCategory>(m_libraryCategoryFilter - 1);
    }

    const auto entries = assetManager.getCatalog().filter(srcFilter, catFilter, m_librarySearchBuf.data());

    ImGui::BeginChild("##LibraryList", ImVec2(0.0f, halfH - 58.0f), true);
    for (const auto* entry : entries) {
        ImGui::PushID(entry->assetId.c_str());
        const bool isProc = (entry->sourceType == assets::AssetSourceType::Procedural);
        ImGui::TextColored(isProc ? ImVec4(0.55f, 0.90f, 0.65f, 1.0f) : ImVec4(0.95f, 0.78f, 0.45f, 1.0f),
                           isProc ? "[P]" : "[B]");
        ImGui::SameLine();
        ImGui::TextUnformatted(entry->displayName.c_str());
        ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - 48.0f);
        if (ImGui::SmallButton("+ Add")) {
            std::string err;
            auto gpuAsset = assetManager.getOrLoadAsset(entry->assetId, &err);
            if (!gpuAsset) {
                m_errorMessage = "Failed to load asset '" + entry->assetId + "': " + err;
            } else {
                const auto newId = systems::PlacementSystem::placeFurniture(
                    m_scene, entry->assetId, assetManager.getCatalog(), &m_selection, m_gridSnapEnabled, &err);
                if (scene::isValidObjectId(newId)) {
                    m_sceneDirty = true;
                    m_errorMessage.clear();
                    m_statusMessage = "Added '" + entry->displayName + "' (ID #" + std::to_string(newId) + ").";
                } else {
                    m_errorMessage = err;
                }
            }
        }
        ImGui::PopID();
    }
    ImGui::EndChild();

    ImGui::Separator();

    // ==================== 2. SCENE HIERARCHY ====================
    ImGui::TextColored(ImVec4(0.45f, 0.80f, 1.0f, 1.0f),
                       "SCENE HIERARCHY (%zu objects)", m_scene.getObjectCount());
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##HierSearch", "Filter objects...", m_hierarchySearchBuf.data(), m_hierarchySearchBuf.size());

    ImGui::BeginChild("##HierarchyTree", ImVec2(0.0f, 0.0f), true);

    if (ImGui::TreeNodeEx("Room Structure", ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto& rs = m_scene.getRoom().getSettings();
        ImGui::BulletText("Floor (%.1f x %.1f m)", rs.width, rs.length);
        ImGui::BulletText("Walls: N:%s S:%s E:%s W:%s",
                          rs.isNorthWallVisible() ? "On" : "Off",
                          rs.isSouthWallVisible() ? "On" : "Off",
                          rs.isEastWallVisible() ? "On" : "Off",
                          rs.isWestWallVisible() ? "On" : "Off");
        ImGui::BulletText("Ceiling: %s", rs.isCeilingVisible() ? "Visible" : "Hidden");
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Furniture Objects", ImGuiTreeNodeFlags_DefaultOpen)) {
        const std::string filterStr(m_hierarchySearchBuf.data());
        scene::ObjectId pendingDeleteId = scene::kInvalidObjectId;
        scene::ObjectId pendingDuplicateId = scene::kInvalidObjectId;

        for (const auto& obj : m_scene.getObjects()) {
            if (!filterStr.empty() && obj.name.find(filterStr) == std::string::npos &&
                obj.assetId.find(filterStr) == std::string::npos) {
                continue;
            }

            // Dùng Stable ObjectId làm ImGui ID để không bao giờ xung đột khi 2 object trùng tên
            ImGui::PushID(static_cast<int>(obj.id));

            bool vis = obj.visible;
            if (ImGui::Checkbox("##vis", &vis)) {
                m_scene.setObjectVisibility(obj.id, vis);
                m_sceneDirty = true;
            }
            ImGui::SameLine();

            const bool selected = m_selection.isSelected(obj.id);
            const std::string label = obj.name + " [#" + std::to_string(obj.id) + "]";
            if (ImGui::Selectable(label.c_str(), selected, ImGuiSelectableFlags_AllowOverlap, ImVec2(ImGui::GetContentRegionAvail().x - 52.0f, 0.0f))) {
                m_selection.selectObject(obj.id);
            }

            ImGui::SameLine();
            if (ImGui::SmallButton("D")) {
                pendingDuplicateId = obj.id;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("X")) {
                pendingDeleteId = obj.id;
            }

            ImGui::PopID();
        }

        if (scene::isValidObjectId(pendingDuplicateId)) {
            std::string err;
            const auto newId = m_scene.duplicateObject(pendingDuplicateId, glm::vec3(0.45f, 0.0f, 0.45f), &err);
            if (scene::isValidObjectId(newId)) {
                m_selection.selectObject(newId);
                m_sceneDirty = true;
                m_statusMessage = "Duplicated object -> ID #" + std::to_string(newId);
            } else {
                m_errorMessage = err;
            }
        }
        if (scene::isValidObjectId(pendingDeleteId)) {
            m_scene.removeObject(pendingDeleteId);
            m_selection.reconcileWithScene(m_scene);
            m_sceneDirty = true;
            m_statusMessage = "Removed object ID #" + std::to_string(pendingDeleteId);
        }

        ImGui::TreePop();
    }

    ImGui::EndChild();
    ImGui::End();
}

void RoomEditor::renderViewportPanel(float posX,
                                     float posY,
                                     float width,
                                     float height,
                                     graphics::Framebuffer& framebuffer,
                                     graphics::Camera& camera)
{
    ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;
    ImGui::Begin("3D Room Viewport", nullptr, flags);

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
        // Lật trục V (uv0=(0,1), uv1=(1,0)) theo quy ước OpenGL -> Dear ImGui
        ImGui::Image(texId, ImVec2(static_cast<float>(targetW), static_cast<float>(targetH)),
                     ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
    }

    ImGui::End();
}

void RoomEditor::renderRightPanel(float posX,
                                  float posY,
                                  float width,
                                  float height,
                                  assets::AssetManager& assetManager,
                                  graphics::Camera& camera)
{
    ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("Inspector & Room Settings", nullptr, flags);

    if (ImGui::BeginTabBar("##RightTabs")) {
        // ==================== TAB 1: PROPERTIES INSPECTOR ====================
        if (ImGui::BeginTabItem("Properties Inspector")) {
            auto* obj = m_selection.hasSelection() ? m_scene.findObject(m_selection.getSelectedObjectId()) : nullptr;
            if (obj == nullptr) {
                ImGui::TextDisabled("No furniture object selected.");
                ImGui::Spacing();
                ImGui::TextWrapped("Select an object in Scene Hierarchy or add one from Furniture Library.");
            } else {
                if (m_renameSyncedObjectId != obj->id) {
                    std::snprintf(m_renameBuf.data(), m_renameBuf.size(), "%s", obj->name.c_str());
                    m_renameSyncedObjectId = obj->id;
                }

                ImGui::Text("Object ID: #%llu", static_cast<unsigned long long>(obj->id));
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 62.0f);
                ImGui::InputText("##ObjName", m_renameBuf.data(), m_renameBuf.size());
                ImGui::SameLine();
                if (ImGui::Button("Rename")) {
                    std::string err;
                    if (m_scene.renameObject(obj->id, m_renameBuf.data(), &err)) {
                        m_sceneDirty = true;
                        m_errorMessage.clear();
                        m_statusMessage = "Renamed object #" + std::to_string(obj->id) + ".";
                    } else {
                        m_errorMessage = err;
                    }
                }

                const auto* catalogEntry = assetManager.getCatalog().findById(obj->assetId);
                ImGui::Text("Asset ID: %s", obj->assetId.c_str());
                if (catalogEntry != nullptr) {
                    ImGui::Text("Source: %s", assets::assetSourceTypeToString(catalogEntry->sourceType));
                }

                bool vis = obj->visible;
                if (ImGui::Checkbox("Visible", &vis)) {
                    m_scene.setObjectVisibility(obj->id, vis);
                    m_sceneDirty = true;
                }

                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.45f, 0.80f, 1.0f, 1.0f), "Transform (drag or double-click)");

                scene::Transform edited = obj->transform;
                bool changed = false;

                const float posSpeed = m_gridSnapEnabled ? 0.10f : 0.05f;
                if (ImGui::DragFloat3("Position", &edited.position.x, posSpeed, -30.0f, 30.0f, "%.3f")) {
                    changed = true;
                }
                if (m_gridSnapEnabled && ImGui::IsItemDeactivatedAfterEdit()) {
                    edited.position = systems::PlacementSystem::snapToGrid(
                        edited.position, m_scene.getRoom().getSettings().gridSpacing, false);
                    changed = true;
                }
                if (ImGui::DragFloat3("Rotation", &edited.rotationDegrees.x, 1.0f, -360.0f, 360.0f, "%.1f")) {
                    changed = true;
                }

                ImGui::Checkbox("Uniform Scale", &m_uniformScale);
                if (m_uniformScale) {
                    float uni = edited.scale.x;
                    if (ImGui::DragFloat("Scale", &uni, 0.02f, 0.05f, 10.0f, "%.3f")) {
                        edited.scale = glm::vec3(uni, uni, uni);
                        changed = true;
                    }
                } else {
                    if (ImGui::DragFloat3("Scale", &edited.scale.x, 0.02f, 0.05f, 10.0f, "%.3f")) {
                        changed = true;
                    }
                }

                if (changed) {
                    std::string err;
                    if (m_scene.setObjectTransform(obj->id, edited, &err)) {
                        m_sceneDirty = true;
                        m_errorMessage.clear();
                    } else {
                        m_errorMessage = err;
                    }
                }

                if (ImGui::Button("Reset Transform")) {
                    scene::Transform resetTr;
                    resetTr.position = glm::vec3(0.0f, (obj->assetId == "blender-carpet") ? 0.003f : 0.0f, 0.0f);
                    m_scene.setObjectTransform(obj->id, resetTr);
                    m_sceneDirty = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Fit Object")) {
                    fitCameraToSelected(assetManager, camera);
                }

                ImGui::Separator();
                auto gpuAsset = assetManager.getOrLoadAsset(obj->assetId, nullptr);
                if (gpuAsset) {
                    const glm::vec3 localSz = gpuAsset->cpuData.localBounds.getSize();
                    const math::BoundingBox worldBox = obj->getWorldBounds(gpuAsset->cpuData.localBounds);
                    const glm::vec3 worldSz = worldBox.getSize();
                    ImGui::Text("Asset Size: %.2f x %.2f x %.2f m", localSz.x, localSz.y, localSz.z);
                    ImGui::Text("World BBox: %.2f x %.2f x %.2f m", worldSz.x, worldSz.y, worldSz.z);

                    if (!m_scene.isBoundsWithinRoom(worldBox)) {
                        ImGui::TextColored(ImVec4(1.0f, 0.72f, 0.25f, 1.0f),
                                           "Warning: Object exceeds room interior bounds.");
                    }
                }
            }
            ImGui::EndTabItem();
        }

        // ==================== TAB 2: ROOM SETTINGS ====================
        if (ImGui::BeginTabItem("Room Settings")) {
            ImGui::TextColored(ImVec4(0.45f, 0.80f, 1.0f, 1.0f), "Room Dimensions (Interior, m)");
            ImGui::DragFloat("Width (X)", &m_roomDraft.width, 0.1f, scene::room_limits::kMinWidth, scene::room_limits::kMaxWidth, "%.2f m");
            ImGui::DragFloat("Length (Z)", &m_roomDraft.length, 0.1f, scene::room_limits::kMinLength, scene::room_limits::kMaxLength, "%.2f m");
            ImGui::DragFloat("Height (Y)", &m_roomDraft.height, 0.1f, scene::room_limits::kMinHeight, scene::room_limits::kMaxHeight, "%.2f m");
            ImGui::DragFloat("Wall Thick", &m_roomDraft.wallThickness, 0.01f, scene::room_limits::kMinWallThickness, scene::room_limits::kMaxWallThickness, "%.2f m");
            ImGui::DragFloat("Grid Step", &m_roomDraft.gridSpacing, 0.05f, scene::room_limits::kMinGridSpacing, scene::room_limits::kMaxGridSpacing, "%.2f m");

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.45f, 0.80f, 1.0f, 1.0f), "Appearance & Visibility");
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
            if (ImGui::Button("Apply Changes")) {
                std::string err;
                if (m_scene.getRoom().applySettings(m_roomDraft, &err)) {
                    m_sceneDirty = true;
                    m_errorMessage.clear();
                    m_statusMessage = "Updated room settings.";
                } else {
                    m_errorMessage = err;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset Room")) {
                m_roomDraft = scene::RoomSettings{};
                std::string err;
                m_scene.getRoom().applySettings(m_roomDraft, &err);
                m_statusMessage = "Reset room settings to defaults.";
            }

            ImGui::Separator();
            const auto& activeRs = m_scene.getRoom().getSettings();
            const auto interior = m_scene.getRoom().getInteriorBounds();
            ImGui::Text("Floor Area : %.2f m^2", activeRs.getFloorArea());
            ImGui::Text("Room Volume: %.2f m^3", activeRs.getVolume());
            ImGui::Text("Objects    : %zu (%zu visible)", m_scene.getObjectCount(), m_scene.getVisibleObjectCount());
            ImGui::Text("X Bounds   : [%.2f, %.2f] m", interior.min.x, interior.max.x);
            ImGui::Text("Z Bounds   : [%.2f, %.2f] m", interior.min.z, interior.max.z);
            ImGui::Text("Y Bounds   : [%.2f, %.2f] m", interior.min.y, interior.max.y);

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    if (!m_errorMessage.empty()) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "Error: %s", m_errorMessage.c_str());
    }

    ImGui::End();
}

void RoomEditor::renderStatusBar(int windowWidth,
                                 float posY,
                                 float height,
                                 const assets::AssetManager& assetManager,
                                 float fps,
                                 float frameTimeMs)
{
    ImGui::SetNextWindowPos(ImVec2(0.0f, posY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(windowWidth), height), ImGuiCond_Always);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoScrollbar;
    ImGui::Begin("##RoomEditorStatusBar", nullptr, flags);

    const auto& m = m_sceneRenderer.getLastMetrics();
    ImGui::Text("FPS: %.1f (%.2f ms) | Objects: %zu/%zu | Unique Assets: %zu (Cache: %zu) | Draws: %u | Tris: %u | %s",
                fps,
                frameTimeMs,
                m.visibleObjects,
                m.totalObjects,
                m.uniqueAssetsUsed,
                assetManager.getCachedAssetCount(),
                m.submittedDrawCalls,
                m.submittedTriangles,
                m_statusMessage.c_str());

    ImGui::End();
}

void RoomEditor::renderConfirmPresetModal(assets::AssetManager& assetManager, graphics::Camera& camera)
{
    if (m_openConfirmModal) {
        ImGui::OpenPopup("Confirm Scene Reset");
        m_openConfirmModal = false;
    }

    if (ImGui::BeginPopupModal("Confirm Scene Reset", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("Your current room scene has unsaved edits. Replace the current scene?");
        ImGui::Spacing();
        if (ImGui::Button("Replace Scene", ImVec2(120.0f, 0.0f))) {
            if (m_pendingNewEmptyRoom) {
                createNewEmptyRoom(camera);
            } else {
                applyPresetImmediate(m_pendingPreset, assetManager, camera);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100.0f, 0.0f))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void RoomEditor::shutdownGpuResources()
{
    m_sceneRenderer.destroyGpuResources();
}

} // namespace roomcraft::editor
