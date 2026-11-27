#pragma once

#include "roomcraft/graphics/Camera.hpp"
#include "roomcraft/graphics/Mesh.hpp"
#include "roomcraft/graphics/Shader.hpp"
#include "roomcraft/graphics/Texture.hpp"
#include "roomcraft/scene/Transform.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <cstdint>

namespace roomcraft::graphics {

// Thống kê thực tế cho từng khung hình render (không hardcode)
struct RenderStats {
    std::uint32_t drawCalls = 0;
    std::uint32_t vertexCount = 0;
    std::uint32_t indexCount = 0;
    std::uint32_t triangleCount = 0;
};

// Lớp Renderer điều phối quá trình vẽ Modern OpenGL 3.3 Core:
// viewport, Depth Testing, Solid/Wireframe (luôn khôi phục GL_FILL ở endFrame),
// vẽ màu cơ bản hoặc kèm Diffuse Texture (M3 Preview) và thống kê draw call thực tế.
class Renderer {
public:
    Renderer() = default;

    void beginFrame(int viewportWidth,
                    int viewportHeight,
                    const glm::vec4& clearColor,
                    bool enableDepthTest = true);

    void setWireframeMode(bool enabled);
    bool isWireframeMode() const { return m_wireframeEnabled; }

    void drawMesh(const Mesh& mesh,
                  Shader& shader,
                  const scene::Transform& transform,
                  const Camera& camera,
                  const glm::vec4& color,
                  float faceContrast = 0.0f);

    void drawMeshWithMatrix(const Mesh& mesh,
                            Shader& shader,
                            const glm::mat4& modelMatrix,
                            const Camera& camera,
                            const glm::vec4& color,
                            float faceContrast = 0.0f);

    // Vẽ Mesh 3D hỗ trợ gắn Diffuse Texture (M3 Asset Preview)
    void drawMeshTextured(const Mesh& mesh,
                          Shader& shader,
                          const glm::mat4& modelMatrix,
                          const Camera& camera,
                          const glm::vec4& color,
                          const Texture* diffuseTexture,
                          float faceContrast = 0.0f);

    void drawLines(const Mesh& lineMesh,
                   Shader& shader,
                   const glm::mat4& modelMatrix,
                   const Camera& camera,
                   const glm::vec4& color);

    void endFrame();

    const RenderStats& getLastFrameStats() const { return m_lastFrameStats; }
    const RenderStats& getCurrentFrameStats() const { return m_currentFrameStats; }

private:
    RenderStats m_currentFrameStats{};
    RenderStats m_lastFrameStats{};
    bool m_wireframeEnabled = false;
};

} // namespace roomcraft::graphics
