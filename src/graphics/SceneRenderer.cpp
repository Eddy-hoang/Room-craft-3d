#include "roomcraft/graphics/SceneRenderer.hpp"

#include "roomcraft/geometry/PrimitiveGenerator.hpp"
#include "roomcraft/systems/TransformGizmo.hpp"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#include <algorithm>
#include <filesystem>
#include <unordered_set>

namespace roomcraft::graphics {

namespace {

constexpr const char* kStudioBgVertFallback = R"(#version 330 core
out vec2 vUv;
void main() {
    vec2 pos = vec2((gl_VertexID == 1) ? 3.0 : -1.0,
                    (gl_VertexID == 2) ? 3.0 : -1.0);
    vUv = pos * 0.5 + 0.5;
    gl_Position = vec4(pos, 0.9999, 1.0);
}
)";

constexpr const char* kStudioBgFragFallback = R"(#version 330 core
in vec2 vUv;
out vec4 FragColor;
void main() {
    vec2 centered = (vUv - vec2(0.5, 0.52)) * vec2(1.15, 1.25);
    float radial = clamp(dot(centered, centered), 0.0, 1.0);
    vec3 bottomCol = vec3(0.082, 0.094, 0.118);
    vec3 topCol = vec3(0.115, 0.130, 0.162);
    vec3 centerGlow = vec3(0.172, 0.194, 0.238);
    vec3 baseGrad = mix(bottomCol, topCol, smoothstep(0.0, 1.0, vUv.y));
    vec3 col = mix(centerGlow, baseGrad, smoothstep(0.0, 0.92, radial));
    float dither = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453) - 0.5;
    FragColor = vec4(clamp(col + vec3(dither / 255.0), 0.0, 1.0), 1.0);
}
)";

} // namespace

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
    Mesh tempBbN, tempBbS, tempBbE, tempBbW, tempWinFrame, tempWinGlass;

    if (!uploadMeshData(room.getFloorMesh(), tempFloor) ||
        !uploadMeshData(room.getNorthWallMesh(), tempNorth) ||
        !uploadMeshData(room.getSouthWallMesh(), tempSouth) ||
        !uploadMeshData(room.getEastWallMesh(), tempEast) ||
        !uploadMeshData(room.getWestWallMesh(), tempWest) ||
        !uploadMeshData(room.getCeilingMesh(), tempCeiling) ||
        !uploadMeshData(room.getGridMesh(), tempGrid)) {
        return false;
    }

    if (!room.getNorthBaseboardMesh().vertices.empty()) uploadMeshData(room.getNorthBaseboardMesh(), tempBbN);
    if (!room.getSouthBaseboardMesh().vertices.empty()) uploadMeshData(room.getSouthBaseboardMesh(), tempBbS);
    if (!room.getEastBaseboardMesh().vertices.empty())  uploadMeshData(room.getEastBaseboardMesh(), tempBbE);
    if (!room.getWestBaseboardMesh().vertices.empty())  uploadMeshData(room.getWestBaseboardMesh(), tempBbW);
    if (!room.getWindowFrameMesh().vertices.empty())    uploadMeshData(room.getWindowFrameMesh(), tempWinFrame);
    if (!room.getWindowGlassMesh().vertices.empty())    uploadMeshData(room.getWindowGlassMesh(), tempWinGlass);

    m_floorMesh = std::move(tempFloor);
    m_northWallMesh = std::move(tempNorth);
    m_southWallMesh = std::move(tempSouth);
    m_eastWallMesh = std::move(tempEast);
    m_westWallMesh = std::move(tempWest);
    m_ceilingMesh = std::move(tempCeiling);
    m_roomGridMesh = std::move(tempGrid);
    m_northBaseboardMesh = std::move(tempBbN);
    m_southBaseboardMesh = std::move(tempBbS);
    m_eastBaseboardMesh = std::move(tempBbE);
    m_westBaseboardMesh = std::move(tempBbW);
    m_windowFrameMesh = std::move(tempWinFrame);
    m_windowGlassMesh = std::move(tempWinGlass);
    m_uploadedRoomRevision = rev;
    return true;
}

bool SceneRenderer::ensureStudioBackground()
{
    if (m_studioBgShader.isValid() && m_studioBgVao != 0) {
        return true;
    }
    if (m_studioBgAttempted) {
        return false;
    }
    m_studioBgAttempted = true;

    std::string err;
    std::error_code ec;
    const bool hasShaderFiles = std::filesystem::exists("assets/shaders/studio_bg.vert", ec) &&
                                std::filesystem::exists("assets/shaders/studio_bg.frag", ec);
    if (!hasShaderFiles ||
        !m_studioBgShader.loadFromFiles("assets/shaders/studio_bg.vert", "assets/shaders/studio_bg.frag", &err)) {
        // Fallback nạp nguồn nhúng khi khởi chạy từ thư mục làm việc khác (ví dụ gói portable ngoài repo)
        m_studioBgShader.loadFromSource(kStudioBgVertFallback, kStudioBgFragFallback, "studio_bg_fallback", &err);
    }
    if (!m_studioBgShader.isValid()) {
        return false;
    }
    if (m_studioBgVao == 0) {
        glGenVertexArrays(1, &m_studioBgVao);
    }
    return m_studioBgVao != 0;
}

void SceneRenderer::renderStudioBackground()
{
    if (!ensureStudioBackground()) {
        return;
    }

    const GLboolean prevDepthTest = glIsEnabled(GL_DEPTH_TEST);
    GLboolean prevDepthMask = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthMask);
    const GLboolean prevCull = glIsEnabled(GL_CULL_FACE);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    m_studioBgShader.bind();
    glBindVertexArray(m_studioBgVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    Shader::unbind();

    glDepthMask(prevDepthMask);
    if (prevDepthTest) glEnable(GL_DEPTH_TEST);
    if (prevCull) glEnable(GL_CULL_FACE);
}

glm::mat4 SceneRenderer::renderShadowPass(const scene::Scene& scene,
                                          assets::AssetManager& assetManager,
                                          Shader& shadowShader,
                                          const LightingSettings& lighting)
{
    const auto& room = scene.getRoom();
    const auto& rs = room.getSettings();
    syncRoomGpuMeshes(room);

    // Tính hộp bao khít nhất gồm các bề mặt phòng đang hiển thị (+ đồ nội thất ở cảnh thiết kế <= 48 vật thể)
    math::BoundingBox casterBounds = room.getVisibleBounds();
    if (scene.getObjectCount() <= 48) {
        for (const auto& obj : scene.getObjects()) {
            if (!obj.visible) continue;
            if (auto gpuAsset = assetManager.getOrLoadAsset(obj.assetId, nullptr)) {
                if (gpuAsset->cpuData.localBounds.isValid()) {
                    casterBounds.expand(obj.getWorldBounds(gpuAsset->cpuData.localBounds));
                }
            }
        }
    }

    const auto frustum = ShadowMap::computeDirectionalLightFrustum(
        lighting.direction, casterBounds, lighting.shadowMapSize);
    m_lastShadowTexelSize = frustum.texelWorldSize;

    if (!lighting.shadowsEnabled || !lighting.sunEnabled || !shadowShader.isValid()) {
        return frustum.lightSpaceMatrix;
    }

    if (!m_shadowMap.create(lighting.shadowMapSize)) {
        return frustum.lightSpaceMatrix;
    }

    // Lưu trạng thái OpenGL trước khi vẽ shadow pass để khôi phục nguyên vẹn
    const GLboolean prevCull = glIsEnabled(GL_CULL_FACE);
    GLint prevCullMode = GL_BACK;
    glGetIntegerv(GL_CULL_FACE_MODE, &prevCullMode);
    const GLboolean prevPolyOffset = glIsEnabled(GL_POLYGON_OFFSET_FILL);

    m_shadowMap.bindForWriting();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.4f, 1.8f);

    shadowShader.bind();
    shadowShader.setMat4("uLightSpaceMatrix", frustum.lightSpaceMatrix);

    const glm::mat4 identity(1.0f);
    shadowShader.setMat4("uModel", identity);

    // 1. Tường phòng là hình hộp đặc khép kín: dùng GL_FRONT culling trong shadow pass giúp
    //    mặt trong của tường KHÔNG tự đổ bóng lên chính nó (triệt tiêu triệt để wall acne / Moiré BUG-01),
    //    đồng thời vẫn giữ nguyên bóng tường đổ vào sàn không bị lọt sáng.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    if (rs.isNorthWallVisible() && m_northWallMesh.isValid()) m_northWallMesh.draw();
    if (rs.isSouthWallVisible() && m_southWallMesh.isValid()) m_southWallMesh.draw();
    if (rs.isWestWallVisible() && m_westWallMesh.isValid()) m_westWallMesh.draw();
    if (rs.isEastWallVisible() && m_eastWallMesh.isValid()) m_eastWallMesh.draw();

    // 2. Đồ nội thất có thể gồm hình học mỏng/hở 1 mặt (chụp đèn, lá cây, miệng bình):
    //    tắt face culling để mọi chi tiết nội thất đều đổ bóng đầy đủ và chống peter-panning.
    glDisable(GL_CULL_FACE);
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

    // Khôi phục chính xác trạng thái OpenGL
    if (!prevPolyOffset) {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
    glCullFace(static_cast<GLenum>(prevCullMode));
    if (prevCull) {
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }

    return frustum.lightSpaceMatrix;
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

    // 0. Nền gradient studio nhẹ (chỉ vẽ khi không ở chế độ wireframe)
    if (options.useStudioBackground && !options.wireframe) {
        renderStudioBackground();
    }

    renderer.setWireframeMode(options.wireframe);

    const glm::mat4 identity(1.0f);

    // 1. Cấu hình ánh sáng, ACES tone mapping, Corner AO theo tường hiển thị & Directional Shadow Map
    renderer.applyLighting(meshShader, camera, options.lighting);
    const bool useShadows = shadowMapReady && (lightSpaceMatrix != nullptr) &&
                            options.lighting.shadowsEnabled && options.lighting.sunEnabled &&
                            !options.wireframe && m_shadowMap.isValid();
    const bool useToneMap = options.useToneMapping && !options.wireframe;

    meshShader.setInt("uToneMapEnabled", useToneMap ? 1 : 0);
    meshShader.setFloat("uExposure", 0.88f);
    meshShader.setVec3("uRoomHalfSize", {rs.width * 0.5f, rs.height, rs.length * 0.5f});
    meshShader.setVec4("uVisibleWalls", {
        rs.isNorthWallVisible() ? 1.0f : 0.0f,
        rs.isSouthWallVisible() ? 1.0f : 0.0f,
        rs.isWestWallVisible()  ? 1.0f : 0.0f,
        rs.isEastWallVisible()  ? 1.0f : 0.0f
    });

    if (useShadows) {
        m_shadowMap.bindDepthTexture(1);
        meshShader.setInt("uShadowMap", 1);
        meshShader.setInt("uShadowEnabled", 1);
        meshShader.setFloat("uShadowBias", options.lighting.shadowBias);
        meshShader.setFloat("uShadowTexelSize", m_lastShadowTexelSize);
        meshShader.setMat4("uLightSpaceMatrix", *lightSpaceMatrix);
    } else {
        meshShader.setInt("uShadowEnabled", 0);
    }

    // 1b. Thu thập tối đa 16 dấu chân đồ nội thất đứng sàn (minY <= 0.05m & cao >= 0.14m -> tự loại thảm & đồ trên bàn)
    //     để đổ Soft Contact Grounding Shadow lên sàn (RC2.4)
    int contactCount = 0;
    if (useToneMap) {
        for (const auto& obj : scene.getObjects()) {
            if (!obj.visible || contactCount >= 16) continue;
            auto gpuAsset = assetManager.getOrLoadAsset(obj.assetId, nullptr);
            if (!gpuAsset || !gpuAsset->cpuData.localBounds.isValid()) continue;
            const auto& lb = gpuAsset->cpuData.localBounds;
            const glm::vec3 scaledSize = lb.getSize() * glm::abs(obj.transform.scale);
            const float worldMinY = obj.transform.position.y + lb.min.y * obj.transform.scale.y;
            // Loại bỏ vật mỏng sát sàn (thảm) và vật đặt trên cao (đèn bàn, máy tính, bình trên bàn trà)
            if (worldMinY > 0.05f || scaledSize.y < 0.14f) continue;

            const glm::vec3 localCenter = lb.getCenter() * obj.transform.scale;
            const float yawRad = glm::radians(obj.transform.rotationDegrees.y);
            const float c = std::cos(yawRad);
            const float s = std::sin(yawRad);
            const float cx = obj.transform.position.x + (localCenter.x * c + localCenter.z * s);
            const float cz = obj.transform.position.z + (-localCenter.x * s + localCenter.z * c);
            // Thu gọn bán kính hộp nhẹ (0.86) để không tạo quầng tối thô tràn ra ngoài mép chân đồ
            const float hx = std::max(0.08f, scaledSize.x * 0.43f);
            const float hz = std::max(0.08f, scaledSize.z * 0.43f);
            const float softRadius = std::clamp(std::min(hx, hz) * 0.45f, 0.08f, 0.22f);
            const float strength = (scaledSize.y > 0.70f) ? 0.30f : 0.22f;

            static const std::string kCeNames[16] = {
                "uContactCenterExt[0]",  "uContactCenterExt[1]",  "uContactCenterExt[2]",  "uContactCenterExt[3]",
                "uContactCenterExt[4]",  "uContactCenterExt[5]",  "uContactCenterExt[6]",  "uContactCenterExt[7]",
                "uContactCenterExt[8]",  "uContactCenterExt[9]",  "uContactCenterExt[10]", "uContactCenterExt[11]",
                "uContactCenterExt[12]", "uContactCenterExt[13]", "uContactCenterExt[14]", "uContactCenterExt[15]"
            };
            static const std::string kBsNames[16] = {
                "uContactBasis[0]",  "uContactBasis[1]",  "uContactBasis[2]",  "uContactBasis[3]",
                "uContactBasis[4]",  "uContactBasis[5]",  "uContactBasis[6]",  "uContactBasis[7]",
                "uContactBasis[8]",  "uContactBasis[9]",  "uContactBasis[10]", "uContactBasis[11]",
                "uContactBasis[12]", "uContactBasis[13]", "uContactBasis[14]", "uContactBasis[15]"
            };
            meshShader.setVec4(kCeNames[contactCount], {cx, cz, hx, hz});
            meshShader.setVec4(kBsNames[contactCount], {c, s, softRadius, strength});
            ++contactCount;
        }
    }
    meshShader.setInt("uContactCount", contactCount);

    // 1c. Vẽ cấu trúc căn phòng (chỉ áp dụng computeRoomCornerAO lên bề mặt phòng — khắc phục BUG-07)
    meshShader.setInt("uIsRoomSurface", 1);
    if (m_floorMesh.isValid()) {
        meshShader.setInt("uFloor", 1);
        meshShader.setVec3("uGrid", {rs.width * 0.5f, rs.length * 0.5f,
                                     options.showGrid && rs.showGrid && !options.wireframe ? rs.gridSpacing : 0.0f});
        static const MaterialData kFloorMat = [] {
            MaterialData m;
            m.specularColor = glm::vec3(0.09f);
            m.shininess = 34.0f;
            return m;
        }();
        renderer.drawMeshTextured(m_floorMesh, meshShader, identity, camera, rs.floorColor, nullptr, options.faceContrast, &kFloorMat);
        ++metrics.roomDrawCalls;
    }
    meshShader.setInt("uFloor", 0);
    meshShader.setInt("uContactCount", 0);

    static const MaterialData kWallMat = [] {
        MaterialData m;
        m.specularColor = glm::vec3(0.03f);
        m.shininess = 14.0f;
        return m;
    }();
    static const MaterialData kTrimMat = [] {
        MaterialData m;
        m.specularColor = glm::vec3(0.06f);
        m.shininess = 28.0f;
        return m;
    }();
    const glm::vec4 baseboardColor = glm::mix(rs.wallColor, glm::vec4(0.95f, 0.94f, 0.91f, 1.0f), 0.55f);

    if (rs.isNorthWallVisible() && m_northWallMesh.isValid()) {
        renderer.drawMeshTextured(m_northWallMesh, meshShader, identity, camera, rs.wallColor, nullptr, options.faceContrast, &kWallMat);
        ++metrics.roomDrawCalls;
        if (m_northBaseboardMesh.isValid()) {
            renderer.drawMeshTextured(m_northBaseboardMesh, meshShader, identity, camera, baseboardColor, nullptr, options.faceContrast, &kTrimMat);
            ++metrics.architecturalDrawCalls;
        }
    }
    if (rs.isSouthWallVisible() && m_southWallMesh.isValid()) {
        renderer.drawMeshTextured(m_southWallMesh, meshShader, identity, camera, rs.wallColor, nullptr, options.faceContrast, &kWallMat);
        ++metrics.roomDrawCalls;
        if (m_southBaseboardMesh.isValid()) {
            renderer.drawMeshTextured(m_southBaseboardMesh, meshShader, identity, camera, baseboardColor, nullptr, options.faceContrast, &kTrimMat);
            ++metrics.architecturalDrawCalls;
        }
    }
    if (rs.isWestWallVisible() && m_westWallMesh.isValid()) {
        renderer.drawMeshTextured(m_westWallMesh, meshShader, identity, camera, rs.wallColor, nullptr, options.faceContrast, &kWallMat);
        ++metrics.roomDrawCalls;
        if (m_westBaseboardMesh.isValid()) {
            renderer.drawMeshTextured(m_westBaseboardMesh, meshShader, identity, camera, baseboardColor, nullptr, options.faceContrast, &kTrimMat);
            ++metrics.architecturalDrawCalls;
        }
    }
    if (rs.isEastWallVisible() && m_eastWallMesh.isValid()) {
        renderer.drawMeshTextured(m_eastWallMesh, meshShader, identity, camera, rs.wallColor, nullptr, options.faceContrast, &kWallMat);
        ++metrics.roomDrawCalls;
        if (m_eastBaseboardMesh.isValid()) {
            renderer.drawMeshTextured(m_eastBaseboardMesh, meshShader, identity, camera, baseboardColor, nullptr, options.faceContrast, &kTrimMat);
            ++metrics.architecturalDrawCalls;
        }
    }
    if (rs.isCeilingVisible() && m_ceilingMesh.isValid()) {
        renderer.drawMeshTextured(m_ceilingMesh, meshShader, identity, camera, rs.ceilingColor, nullptr, options.faceContrast, &kWallMat);
        ++metrics.roomDrawCalls;
    }

    // 1d. Cửa sổ kiến trúc trang trí trên tường Bắc (RC2.5 — tôn trọng visibility tường Bắc & màu trời theo ánh sáng)
    if (rs.showWindow && rs.isNorthWallVisible() && m_windowFrameMesh.isValid() && m_windowGlassMesh.isValid()) {
        meshShader.setInt("uIsRoomSurface", 0);
        const glm::vec4 frameCol(0.93f, 0.92f, 0.89f, 1.0f);
        renderer.drawMeshTextured(m_windowFrameMesh, meshShader, identity, camera, frameCol, nullptr, options.faceContrast, &kTrimMat);
        ++metrics.architecturalDrawCalls;

        // Kính cửa sổ phản chiếu bầu trời dịu thích nghi theo tông màu nguồn sáng hiện tại
        glm::vec4 glassTint(0.66f, 0.78f, 0.89f, 1.0f); // Daylight sky
        if (options.lighting.ambient < 0.20f) {
            glassTint = glm::vec4(0.18f, 0.25f, 0.42f, 1.0f); // Evening twilight
        } else if (options.lighting.pointEnabled) {
            glassTint = glm::vec4(0.84f, 0.78f, 0.70f, 1.0f); // Warm late afternoon
        }
        renderer.drawMeshTextured(m_windowGlassMesh, meshShader, identity, camera, glassTint, nullptr, 0.0f);
        ++metrics.architecturalDrawCalls;
    }
    meshShader.setInt("uIsRoomSurface", 0);

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

    // Tắt trạng thái shadow & tone mapping trước khi vẽ overlays/gizmo/thumbnail kế tiếp
    meshShader.bind();
    meshShader.setInt("uShadowEnabled", 0);
    meshShader.setInt("uToneMapEnabled", 0);
    meshShader.setInt("uIsRoomSurface", 0);
    meshShader.setVec3("uRoomHalfSize", glm::vec3(0.0f));
    if (useShadows) {
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
                           glm::vec4(0.20f, 0.88f, 1.00f, 1.0f));
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
            if (hasSelectedToHighlight && selection.isSelected(obj.id)) continue;

            auto gpuAsset = assetManager.getOrLoadAsset(obj.assetId, nullptr);
            if (!gpuAsset || !gpuAsset->boundingBoxLines.isValid()) continue;

            const glm::vec4 warnColor = isOutOfBounds
                                            ? glm::vec4(0.92f, 0.36f, 0.36f, 1.0f)
                                            : glm::vec4(0.92f, 0.70f, 0.32f, 1.0f);
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
            baseColor = glm::vec4(0.98f, 0.84f, 0.34f, 1.0f);
        }
        const glm::mat4 base = glm::translate(glm::mat4(1.0f), origin) * rotMat *
                               glm::scale(glm::mat4(1.0f), glm::vec3(len));
        const glm::mat4 tip = glm::translate(glm::mat4(1.0f), origin) * rotMat *
                              glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.85f * len, 0.0f)) *
                              glm::scale(glm::mat4(1.0f), glm::vec3(len));
        renderer.drawMeshWithMatrix(m_gizmoShaftMesh, meshShader, base, camera, baseColor, 0.0f);
        renderer.drawMeshWithMatrix(m_gizmoConeMesh, meshShader, tip, camera, baseColor, 0.0f);
    };

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
    m_northBaseboardMesh.destroy();
    m_southBaseboardMesh.destroy();
    m_eastBaseboardMesh.destroy();
    m_westBaseboardMesh.destroy();
    m_windowFrameMesh.destroy();
    m_windowGlassMesh.destroy();
    m_gizmoShaftMesh.destroy();
    m_gizmoConeMesh.destroy();
    m_shadowMap.destroy();
    m_studioBgShader.destroy();
    if (m_studioBgVao != 0) {
        glDeleteVertexArrays(1, &m_studioBgVao);
        m_studioBgVao = 0;
    }
    m_studioBgAttempted = false;
    m_uploadedRoomRevision = 0;
}

} // namespace roomcraft::graphics
