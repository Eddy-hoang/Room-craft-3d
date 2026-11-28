#pragma once

#include "roomcraft/assets/AssetManager.hpp"
#include "roomcraft/graphics/Camera.hpp"
#include "roomcraft/graphics/Mesh.hpp"
#include "roomcraft/graphics/Renderer.hpp"
#include "roomcraft/graphics/Shader.hpp"
#include "roomcraft/graphics/ShadowMap.hpp"
#include "roomcraft/scene/Scene.hpp"
#include "roomcraft/systems/InteractionState.hpp"
#include "roomcraft/systems/SelectionState.hpp"

#include <glm/vec4.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace roomcraft::graphics {

struct SceneRenderOptions {
    bool wireframe = false;
    bool depthTest = true;
    bool showGrid = true;
    bool showAxes = true;
    bool showSelectedBoundingBox = true;
    bool showMoveGizmo = false;
    systems::AxisConstraint activeGizmoAxis = systems::AxisConstraint::None;
    systems::AxisConstraint hoveredGizmoAxis = systems::AxisConstraint::None;
    LightingSettings lighting;
    float faceContrast = 0.35f;
    // Phản hồi trực quan va chạm & vượt biên phòng (M8.4)
    std::vector<scene::ObjectId> collidingObjectIds;
    std::vector<scene::ObjectId> outOfBoundsObjectIds;
};

// Thống kê render thực tế của SceneRenderer — phân biệt rõ dữ liệu lưu trên GPU dùng chung
// (unique GPU vertices/indices) với lượng đỉnh/tam giác gửi qua nhiều draw call cho các instance.
struct SceneRenderMetrics {
    std::size_t totalObjects = 0;
    std::size_t visibleObjects = 0;
    std::size_t uniqueAssetsUsed = 0;
    std::uint32_t uniqueGpuVertices = 0;
    std::uint32_t uniqueGpuIndices = 0;
    std::uint32_t uniqueGpuTriangles = 0;
    std::uint32_t roomDrawCalls = 0;
    std::uint32_t objectDrawCalls = 0;
    std::uint32_t submittedDrawCalls = 0;
    std::uint32_t submittedVertices = 0;
    std::uint32_t submittedIndices = 0;
    std::uint32_t submittedTriangles = 0;
};

// Lớp SceneRenderer kết nối dữ liệu thuần CPU (Scene, Room, SceneObject, SelectionState)
// với tầng GPU (AssetManager, Renderer, Shader, Camera) để vẽ toàn bộ căn phòng và nhiều vật thể.
class SceneRenderer {
public:
    SceneRenderer() = default;
    ~SceneRenderer();

    SceneRenderer(const SceneRenderer&) = delete;
    SceneRenderer& operator=(const SceneRenderer&) = delete;

    // Đồng bộ mesh CPU của Room lên GPU CHỈ KHI geometryRevision thay đổi (không upload mỗi frame)
    bool syncRoomGpuMeshes(const scene::Room& room);

    // Vẽ pass độ sâu cho Directional Shadow Mapping (M9.4) trước khi bind FBO chính
    glm::mat4 renderShadowPass(const scene::Scene& scene,
                               assets::AssetManager& assetManager,
                               Shader& shadowShader,
                               const LightingSettings& lighting);

    // Vẽ toàn bộ Scene vào Render Target hiện tại (FBO đã bind trước)
    SceneRenderMetrics renderScene(const scene::Scene& scene,
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
                                   const glm::mat4* lightSpaceMatrix = nullptr,
                                   bool shadowMapReady = false);

    const SceneRenderMetrics& getLastMetrics() const { return m_lastMetrics; }

    // Giải phóng toàn bộ GPU mesh của Room trước khi OpenGL context bị hủy
    void destroyGpuResources();

private:
    static bool uploadMeshData(const geometry::MeshData& src, Mesh& dst);
    bool ensureGizmoMeshes();
    void renderMoveGizmo(const glm::vec3& origin,
                         bool locked,
                         const Camera& camera,
                         Renderer& renderer,
                         Shader& meshShader,
                         const SceneRenderOptions& options);

private:
    Mesh m_floorMesh;
    Mesh m_northWallMesh;
    Mesh m_southWallMesh;
    Mesh m_eastWallMesh;
    Mesh m_westWallMesh;
    Mesh m_ceilingMesh;
    Mesh m_roomGridMesh;
    Mesh m_gizmoShaftMesh;
    Mesh m_gizmoConeMesh;
    ShadowMap m_shadowMap;
    std::uint64_t m_uploadedRoomRevision = 0;
    SceneRenderMetrics m_lastMetrics{};
};

} // namespace roomcraft::graphics
