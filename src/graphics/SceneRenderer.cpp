#include "roomcraft/graphics/SceneRenderer.hpp"

#include "roomcraft/geometry/PrimitiveGenerator.hpp"
#include "roomcraft/systems/TransformGizmo.hpp"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#include <algorithm>
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

glm::mat4 SceneRenderer::renderShadowPass(const scene::Scene& scene,
                                          assets::AssetManager& assetManager,
                                          Shader& shadowShader,
                                          const LightingSettings& lighting)
{
    const auto& room = scene.getRoom();
    const auto& rs = room.getSettings();
    syncRoomGpuMeshes(room);

    const glm::mat4 lightSpace = ShadowMap::computeDirectionalLightSpaceMatrix(
        lighting.direction, room.getExteriorBounds());

    if (!lighting.shadowsEnabled || !lighting.sunEnabled || !shadowShader.isValid()) {
        return lightSpace;
    }

    if (!m_shadowMap.create(lighting.shadowMapSize)) {
        return lightSpace;
    }

    m_shadowMap.bindForWriting();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    shadowShader.bind();
    shadowShader.setMat4("uLightSpaceMatrix", lightSpace);

    const glm::mat4 identity(1.0f);
    shadowShader.setMat4("uModel", identity);

    // Các bức tường đang hiển thị đổ bóng xuống sàn (không vẽ sàn trong pass bóng để tránh self-acne sàn)
    if (rs.isNorthWallVisible() && m_northWallMesh.isValid()) m_northWallMesh.draw();
    if (rs.isSouthWallVisible() && m_southWallMesh.isValid()) m_southWallMesh.draw();
    if (rs.isWestWallVisible() && m_westWallMesh.isValid()) m_westWallMesh.draw();
    if (rs.isEastWallVisible() && m_eastWallMesh.isValid()) m_eastWallMesh.draw();

    // Toàn bộ nội thất đang hiển thị đổ bóng lên sàn và các đồ nội thất khác
    for (const auto& obj : scene.getObjects()) {
        if (!obj.visible) {
            continue;
        }
        auto gpuAsset = assetManager.getOrLoadAsset(obj.assetId, nullptr);
        if (!gpuAsset) {
            continue;
        }
        const glm::mat4 objectMatrix = obj.getModelMatrix();
        for (const auto& sub : gpuAsset->subMeshes) {
            shadowShader.setMat4("uModel", objectMatrix * sub.localTransform.getModelMatrix());
            sub.mesh.draw();
        }
    }

    Shader::unbind();
    ShadowMap::unbind();
    return lightSpace;
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
                                              const SceneRenderOptions& options,
                                              const glm::mat4* lightSpaceMatrix,
                                              bool shadowMapReady)
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
    renderer.applyLighting(meshShader, camera, options.lighting);
    const bool useShadows = shadowMapReady && (lightSpaceMatrix != nullptr) &&
                            options.lighting.shadowsEnabled && options.lighting.sunEnabled &&
                            !options.wireframe && m_shadowMap.isValid();
    if (useShadows) {
        m_shadowMap.bindDepthTexture(1);
        meshShader.setInt("uShadowMap", 1);
        meshShader.setInt("uShadowEnabled", 1);
        meshShader.setFloat("uShadowBias", options.lighting.shadowBias);
        meshShader.setMat4("uLightSpaceMatrix", *lightSpaceMatrix);
        meshShader.setVec3("uRoomHalfSize", {rs.width * 0.5f, rs.height, rs.length * 0.5f});
    } else {
        meshShader.setInt("uShadowEnabled", 0);
        meshShader.setVec3("uRoomHalfSize", glm::vec3(0.0f));
    }
    if (m_floorMesh.isValid()) {
        meshShader.setInt("uFloor", 1);
        meshShader.setVec3("uGrid", {rs.width * 0.5f, rs.length * 0.5f,
                                     options.showGrid && rs.showGrid && !options.wireframe ? rs.gridSpacing : 0.0f});
        renderer.drawMeshWithMatrix(m_floorMesh, meshShader, identity, camera, rs.floorColor, options.faceContrast);
        ++metrics.roomDrawCalls;
    }
    meshShader.setInt("uFloor", 0);
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
                                      options.faceContrast,
                                      sub.materialIndex < gpuAsset->cpuData.materials.size()
                                          ? &gpuAsset->cpuData.materials[sub.materialIndex] : nullptr,
                                      obj.shininessScale);
            ++metrics.objectDrawCalls;
        }

        if (options.showSelectedBoundingBox && selection.isSelected(obj.id)) {
            selectedAsset = gpuAsset;
            selectedObjectMatrix = objectMatrix;
            hasSelectedToHighlight = true;
        }
    }

    metrics.uniqueAssetsUsed = seenAssets.size();

    if (useShadows) {
        meshShader.bind();
        meshShader.setInt("uShadowEnabled", 0);
        meshShader.setVec3("uRoomHalfSize", glm::vec3(0.0f));
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
    }

    // 3. Vẽ lưới sàn, trục tọa độ và hộp bao nổi bật của đối tượng đang chọn (luôn ở chế độ GL_FILL)
    renderer.setWireframeMode(false);

    if (options.wireframe && options.showGrid && rs.showGrid && m_roomGridMesh.isValid()) {
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
                           glm::vec4(0.20f, 0.88f, 1.00f, 1.0f)); // Cyan selection bounding box highlight
    }

    // 3b. Vẽ khung cảnh báo va chạm (hổ phách) và vượt biên phòng (đỏ san hô) cho các vật thể vi phạm (M8.4)
    if (!options.collidingObjectIds.empty() || !options.outOfBoundsObjectIds.empty()) {
        auto containsId = [](const std::vector<scene::ObjectId>& ids, scene::ObjectId target) {
            return std::find(ids.begin(), ids.end(), target) != ids.end();
        };
        for (const auto& obj : scene.getObjects()) {
            if (!obj.visible) continue;
            const bool isOutOfBounds = containsId(options.outOfBoundsObjectIds, obj.id);
            const bool isColliding = containsId(options.collidingObjectIds, obj.id);
            if (!isOutOfBounds && !isColliding) continue;
            // Không ghi đè đường viền cyan của đối tượng đang chọn để giữ rõ ràng (và bảo toàn TC-M5-38)
            if (hasSelectedToHighlight && selection.isSelected(obj.id)) continue;

            auto gpuAsset = assetManager.getOrLoadAsset(obj.assetId, nullptr);
            if (!gpuAsset || !gpuAsset->boundingBoxLines.isValid()) continue;

            const glm::vec4 warnColor = isOutOfBounds
                                            ? glm::vec4(0.92f, 0.36f, 0.36f, 1.0f)  // Đỏ san hô: vượt biên phòng
                                            : glm::vec4(0.92f, 0.70f, 0.32f, 1.0f); // Hổ phách: va chạm nội thất
            renderer.drawLines(gpuAsset->boundingBoxLines, gridShader, obj.getModelMatrix(), camera, warnColor);
        }
    }

    // 4. Vẽ Move Gizmo 3 trục (X Đỏ, Y Xanh lá, Z Xanh dương) tại tâm đối tượng đang chọn
    if (options.showMoveGizmo && selection.hasSelection()) {
        if (const auto* selObj = scene.findObject(selection.getSelectedObjectId())) {
            renderMoveGizmo(selObj->transform.position, selObj->locked, camera, renderer, meshShader, options);
        }
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

bool SceneRenderer::ensureGizmoMeshes()
{
    if (m_gizmoShaftMesh.isValid() && m_gizmoConeMesh.isValid()) {
        return true;
    }
    geometry::MeshData shaftData;
    geometry::MeshData coneData;
    if (!geometry::PrimitiveGenerator::createCylinder(0.024f, 0.85f, 12, true, shaftData) ||
        !geometry::PrimitiveGenerator::createCone(0.068f, 0.24f, 12, true, coneData)) {
        return false;
    }
    return uploadMeshData(shaftData, m_gizmoShaftMesh) && uploadMeshData(coneData, m_gizmoConeMesh);
}

void SceneRenderer::renderMoveGizmo(const glm::vec3& origin,
                                    bool locked,
                                    const Camera& camera,
                                    Renderer& renderer,
                                    Shader& meshShader,
                                    const SceneRenderOptions& options)
{
    if (!ensureGizmoMeshes()) {
        return;
    }

    const float len = systems::TransformGizmo::computeHandleLength(origin, camera);
    const GLboolean prevDepth = glIsEnabled(GL_DEPTH_TEST);
    glDisable(GL_DEPTH_TEST);

    auto drawAxis = [&](systems::AxisConstraint axis, const glm::mat4& rotMat, glm::vec4 baseColor) {
        if (locked) {
            baseColor = glm::vec4(0.55f, 0.58f, 0.64f, 1.0f);
        } else if (options.activeGizmoAxis == axis || options.hoveredGizmoAxis == axis) {
            baseColor = glm::vec4(0.98f, 0.84f, 0.34f, 1.0f); // Highlight vàng sáng khi hover / kéo trục
        }
        const glm::mat4 base = glm::translate(glm::mat4(1.0f), origin) * rotMat *
                               glm::scale(glm::mat4(1.0f), glm::vec3(len));
        const glm::mat4 tip = glm::translate(glm::mat4(1.0f), origin) * rotMat *
                              glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.85f * len, 0.0f)) *
                              glm::scale(glm::mat4(1.0f), glm::vec3(len));
        renderer.drawMeshWithMatrix(m_gizmoShaftMesh, meshShader, base, camera, baseColor, 0.0f);
        renderer.drawMeshWithMatrix(m_gizmoConeMesh, meshShader, tip, camera, baseColor, 0.0f);
    };

    // Mặc định Cylinder/Cone hướng +Y; xoay sang +X (-90° quanh Z) và +Z (+90° quanh X)
    const glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    const glm::mat4 rotY(1.0f);
    const glm::mat4 rotZ = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    drawAxis(systems::AxisConstraint::X, rotX, glm::vec4(0.89f, 0.36f, 0.36f, 1.0f));
    drawAxis(systems::AxisConstraint::Y, rotY, glm::vec4(0.37f, 0.79f, 0.48f, 1.0f));
    drawAxis(systems::AxisConstraint::Z, rotZ, glm::vec4(0.36f, 0.61f, 0.92f, 1.0f));

    if (prevDepth) {
        glEnable(GL_DEPTH_TEST);
    }
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
    m_gizmoShaftMesh.destroy();
    m_gizmoConeMesh.destroy();
    m_shadowMap.destroy();
    m_uploadedRoomRevision = 0;
}

} // namespace roomcraft::graphics
