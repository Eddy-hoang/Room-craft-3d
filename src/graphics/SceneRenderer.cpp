#include "roomcraft/graphics/SceneRenderer.hpp"

#include <glm/mat4x4.hpp>

#include <unordered_set>

namespace roomcraft::graphics {

SceneRenderer::~SceneRenderer()
{
    destroyGpuResources();
}

bool SceneRenderer::uploadMeshData(const geometry::MeshData& src, Mesh& dst)
{
    return dst.create(src.vertices, src.indices, src.topology);
}

bool SceneRenderer::syncRoomGpuMeshes(const scene::Room& room)
{
    const std::uint64_t rev = room.getGeometryRevision();
    if (rev == m_uploadedRoomRevision && m_floorMesh.isValid()) {
        return true;
    }

    // Tạo tạm trước rồi mới hoán đổi (transactional GPU upload)
    Mesh tempFloor;
    Mesh tempNorth;
    Mesh tempSouth;
    Mesh tempEast;
    Mesh tempWest;
    Mesh tempCeiling;
    Mesh tempGrid;

    if (!uploadMeshData(room.getFloorMesh(), tempFloor) ||
        !uploadMeshData(room.getNorthWallMesh(), tempNorth) ||
        !uploadMeshData(room.getSouthWallMesh(), tempSouth) ||
        !uploadMeshData(room.getEastWallMesh(), tempEast) ||
        !uploadMeshData(room.getWestWallMesh(), tempWest) ||
        !uploadMeshData(room.getCeilingMesh(), tempCeiling) ||
        !uploadMeshData(room.getGridMesh(), tempGrid)) {
        return false;
    }

    m_floorMesh = std::move(tempFloor);
    m_northWallMesh = std::move(tempNorth);
    m_southWallMesh = std::move(tempSouth);
    m_eastWallMesh = std::move(tempEast);
    m_westWallMesh = std::move(tempWest);
    m_ceilingMesh = std::move(tempCeiling);
    m_roomGridMesh = std::move(tempGrid);
    m_uploadedRoomRevision = rev;
    return true;
}

SceneRenderMetrics SceneRenderer::renderScene(const scene::Scene& scene,
                                              assets::AssetManager& assetManager,
                                              const systems::SelectionState& selection,
                                              const Camera& camera,
                                              Renderer& renderer,
                                              Shader& meshShader,
                                              Shader& gridShader,
                                              const Mesh* axisXMesh,
                                              const Mesh* axisYMesh,
                                              const Mesh* axisZMesh,
                                              int viewportWidth,
                                              int viewportHeight,
                                              const glm::vec4& clearColor,
                                              const SceneRenderOptions& options)
{
    SceneRenderMetrics metrics{};
    metrics.totalObjects = scene.getObjectCount();

    const auto& room = scene.getRoom();
    const auto& rs = room.getSettings();
    syncRoomGpuMeshes(room);

    renderer.beginFrame(viewportWidth, viewportHeight, clearColor, options.depthTest);
    renderer.setWireframeMode(options.wireframe);

    const glm::mat4 identity(1.0f);

    // 1. Vẽ cấu trúc căn phòng (Sàn, 4 tường theo WallVisibilityMode, Trần tùy chọn)
    if (m_floorMesh.isValid()) {
        renderer.drawMeshWithMatrix(m_floorMesh, meshShader, identity, camera, rs.floorColor, options.faceContrast);
        ++metrics.roomDrawCalls;
    }
    if (rs.isNorthWallVisible() && m_northWallMesh.isValid()) {
        renderer.drawMeshWithMatrix(m_northWallMesh, meshShader, identity, camera, rs.wallColor, options.faceContrast);
        ++metrics.roomDrawCalls;
    }
    if (rs.isSouthWallVisible() && m_southWallMesh.isValid()) {
        renderer.drawMeshWithMatrix(m_southWallMesh, meshShader, identity, camera, rs.wallColor, options.faceContrast);
        ++metrics.roomDrawCalls;
    }
    if (rs.isWestWallVisible() && m_westWallMesh.isValid()) {
        renderer.drawMeshWithMatrix(m_westWallMesh, meshShader, identity, camera, rs.wallColor, options.faceContrast);
        ++metrics.roomDrawCalls;
    }
    if (rs.isEastWallVisible() && m_eastWallMesh.isValid()) {
        renderer.drawMeshWithMatrix(m_eastWallMesh, meshShader, identity, camera, rs.wallColor, options.faceContrast);
        ++metrics.roomDrawCalls;
    }
    if (rs.isCeilingVisible() && m_ceilingMesh.isValid()) {
        renderer.drawMeshWithMatrix(m_ceilingMesh, meshShader, identity, camera, rs.ceilingColor, options.faceContrast);
        ++metrics.roomDrawCalls;
    }

    // 2. Vẽ toàn bộ SceneObject đang hiển thị (dùng chung GpuAsset từ AssetManager)
    std::unordered_set<std::string> seenAssets;
    std::shared_ptr<assets::GpuAsset> selectedAsset;
    glm::mat4 selectedObjectMatrix(1.0f);
    bool hasSelectedToHighlight = false;

    for (const auto& obj : scene.getObjects()) {
        if (!obj.visible) {
            continue;
        }
        ++metrics.visibleObjects;

        // Luôn resolve qua AssetManager (trả về bản cache nhanh, an toàn ngay cả sau reloadAsset)
        auto gpuAsset = assetManager.getOrLoadAsset(obj.assetId, nullptr);
        if (!gpuAsset) {
            continue;
        }

        if (seenAssets.insert(obj.assetId).second) {
            metrics.uniqueGpuVertices += gpuAsset->cpuData.getTotalVertexCount();
            metrics.uniqueGpuIndices += gpuAsset->cpuData.getTotalIndexCount();
            metrics.uniqueGpuTriangles += gpuAsset->cpuData.getTotalTriangleCount();
        }

        const glm::mat4 objectMatrix = obj.getModelMatrix();
        for (const auto& sub : gpuAsset->subMeshes) {
            // Công thức biến đổi phân cấp: M_world_part = M_object * M_asset_part
            // (Với asset Blender, node transform đã được bake ở ModelLoader nên localTransform là Identity)
            const glm::mat4 worldPartMatrix = objectMatrix * sub.localTransform.getModelMatrix();

            glm::vec4 color(0.80f, 0.80f, 0.80f, 1.0f);
            const Texture* diffuseTex = nullptr;
            if (sub.materialIndex < gpuAsset->cpuData.materials.size()) {
                color = gpuAsset->cpuData.materials[sub.materialIndex].baseColor;
            }
            if (sub.materialIndex < gpuAsset->materialDiffuseTextures.size() &&
                gpuAsset->materialDiffuseTextures[sub.materialIndex] != nullptr) {
                diffuseTex = gpuAsset->materialDiffuseTextures[sub.materialIndex].get();
            }

            renderer.drawMeshTextured(sub.mesh,
                                      meshShader,
                                      worldPartMatrix,
                                      camera,
                                      color,
                                      diffuseTex,
                                      options.faceContrast);
            ++metrics.objectDrawCalls;
        }

        if (options.showSelectedBoundingBox && selection.isSelected(obj.id)) {
            selectedAsset = gpuAsset;
            selectedObjectMatrix = objectMatrix;
            hasSelectedToHighlight = true;
        }
    }

    metrics.uniqueAssetsUsed = seenAssets.size();

    // 3. Vẽ lưới sàn, trục tọa độ và hộp bao nổi bật của đối tượng đang chọn (luôn ở chế độ GL_FILL)
    renderer.setWireframeMode(false);

    if (options.showGrid && rs.showGrid && m_roomGridMesh.isValid()) {
        renderer.drawLines(m_roomGridMesh, gridShader, identity, camera, glm::vec4(0.32f, 0.36f, 0.44f, 0.70f));
    }

    if (options.showAxes) {
        if (axisXMesh != nullptr && axisXMesh->isValid()) {
            renderer.drawLines(*axisXMesh, gridShader, identity, camera, glm::vec4(0.95f, 0.25f, 0.25f, 1.0f));
        }
        if (axisYMesh != nullptr && axisYMesh->isValid()) {
            renderer.drawLines(*axisYMesh, gridShader, identity, camera, glm::vec4(0.25f, 0.90f, 0.35f, 1.0f));
        }
        if (axisZMesh != nullptr && axisZMesh->isValid()) {
            renderer.drawLines(*axisZMesh, gridShader, identity, camera, glm::vec4(0.25f, 0.55f, 0.98f, 1.0f));
        }
    }

    if (hasSelectedToHighlight && selectedAsset && selectedAsset->boundingBoxLines.isValid()) {
        renderer.drawLines(selectedAsset->boundingBoxLines,
                           gridShader,
                           selectedObjectMatrix,
                           camera,
                           glm::vec4(0.20f, 0.88f, 1.00f, 1.0f));
    }

    renderer.endFrame();

    const auto& frameStats = renderer.getLastFrameStats();
    metrics.submittedDrawCalls = frameStats.drawCalls;
    metrics.submittedVertices = frameStats.vertexCount;
    metrics.submittedIndices = frameStats.indexCount;
    metrics.submittedTriangles = frameStats.triangleCount;

    m_lastMetrics = metrics;
    return metrics;
}

void SceneRenderer::destroyGpuResources()
{
    m_floorMesh.destroy();
    m_northWallMesh.destroy();
    m_southWallMesh.destroy();
    m_eastWallMesh.destroy();
    m_westWallMesh.destroy();
    m_ceilingMesh.destroy();
    m_roomGridMesh.destroy();
    m_uploadedRoomRevision = 0;
}

} // namespace roomcraft::graphics
