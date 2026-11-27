#pragma once

#include "roomcraft/graphics/Camera.hpp"
#include "roomcraft/graphics/Mesh.hpp"
#include "roomcraft/graphics/Shader.hpp"
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
};

// Lớp Renderer điều phối quá trình vẽ Modern OpenGL 3.3 Core:
// thiết lập viewport, bật/tắt Depth Testing, xóa buffer, truyền MVP uniform và ghi nhận thống kê draw call.
class Renderer {
public:
    Renderer() = default;

    // Bắt đầu khung hình mới: thiết lập viewport, trạng thái Depth Test và xóa Color + Depth Buffer
    void beginFrame(int viewportWidth,
                    int viewportHeight,
                    const glm::vec4& clearColor,
                    bool enableDepthTest = true);

    // Vẽ một Mesh 3D với Shader, Transform (Model) và Camera (View/Projection)
    void drawMesh(const Mesh& mesh,
                  Shader& shader,
                  const scene::Transform& transform,
                  const Camera& camera,
                  const glm::vec4& color,
                  float faceContrast = 0.0f);

    // Vẽ lưới / trục tọa độ (GL_LINES) bằng ma trận Model trực tiếp
    void drawLines(const Mesh& lineMesh,
                   Shader& shader,
                   const glm::mat4& modelMatrix,
                   const Camera& camera,
                   const glm::vec4& color);

    // Kết thúc khung hình, chốt thống kê và giải phóng trạng thái shader
    void endFrame();

    const RenderStats& getLastFrameStats() const { return m_lastFrameStats; }
    const RenderStats& getCurrentFrameStats() const { return m_currentFrameStats; }

private:
    RenderStats m_currentFrameStats{};
    RenderStats m_lastFrameStats{};
};

} // namespace roomcraft::graphics
