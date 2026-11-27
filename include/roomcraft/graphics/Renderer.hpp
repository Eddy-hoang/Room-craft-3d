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
    std::uint32_t triangleCount = 0;
};

// Lớp Renderer điều phối quá trình vẽ Modern OpenGL 3.3 Core:
// thiết lập viewport, bật/tắt Depth Testing, chế độ Solid/Wireframe (luôn khôi phục GL_FILL ở endFrame),
// truyền MVP uniform và ghi nhận thống kê draw call thực tế.
class Renderer {
public:
    Renderer() = default;

    // Bắt đầu khung hình mới: thiết lập viewport, Depth Test và xóa Color + Depth Buffer
    void beginFrame(int viewportWidth,
                    int viewportHeight,
                    const glm::vec4& clearColor,
                    bool enableDepthTest = true);

    // Bật/tắt chế độ khung dây (GL_LINE vs GL_FILL); tự động khôi phục về GL_FILL khi gọi endFrame()
    void setWireframeMode(bool enabled);
    bool isWireframeMode() const { return m_wireframeEnabled; }

    // Vẽ một Mesh 3D bằng Transform (Model = T * R * S)
    void drawMesh(const Mesh& mesh,
                  Shader& shader,
                  const scene::Transform& transform,
                  const Camera& camera,
                  const glm::vec4& color,
                  float faceContrast = 0.0f);

    // Vẽ một Mesh 3D bằng ma trận Model trực tiếp (phục vụ mô hình phân cấp M_parent * M_local)
    void drawMeshWithMatrix(const Mesh& mesh,
                            Shader& shader,
                            const glm::mat4& modelMatrix,
                            const Camera& camera,
                            const glm::vec4& color,
                            float faceContrast = 0.0f);

    // Vẽ lưới / trục tọa độ / đường cong / pháp tuyến (GL_LINES)
    void drawLines(const Mesh& lineMesh,
                   Shader& shader,
                   const glm::mat4& modelMatrix,
                   const Camera& camera,
                   const glm::vec4& color);

    // Kết thúc khung hình, khôi phục GL_FILL, unbind shader và chốt thống kê
    void endFrame();

    const RenderStats& getLastFrameStats() const { return m_lastFrameStats; }
    const RenderStats& getCurrentFrameStats() const { return m_currentFrameStats; }

private:
    RenderStats m_currentFrameStats{};
    RenderStats m_lastFrameStats{};
    bool m_wireframeEnabled = false;
};

} // namespace roomcraft::graphics
