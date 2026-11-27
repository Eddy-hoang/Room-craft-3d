#include "roomcraft/graphics/Renderer.hpp"

#include <glad/glad.h>

namespace roomcraft::graphics {

void Renderer::beginFrame(int viewportWidth,
                          int viewportHeight,
                          const glm::vec4& clearColor,
                          bool enableDepthTest)
{
    m_currentFrameStats = RenderStats{};

    if (viewportWidth > 0 && viewportHeight > 0) {
        glViewport(0, 0, viewportWidth, viewportHeight);
    }

    if (enableDepthTest) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::drawMesh(const Mesh& mesh,
                        Shader& shader,
                        const scene::Transform& transform,
                        const Camera& camera,
                        const glm::vec4& color,
                        float faceContrast)
{
    if (!mesh.isValid() || !shader.isValid()) {
        return;
    }

    shader.bind();
    shader.setMat4("uModel", transform.getModelMatrix());
    shader.setMat4("uView", camera.getViewMatrix());
    shader.setMat4("uProjection", camera.getProjectionMatrix());
    shader.setVec4("uColor", color);
    shader.setFloat("uFaceContrast", faceContrast);

    mesh.draw();

    ++m_currentFrameStats.drawCalls;
    m_currentFrameStats.vertexCount += mesh.getVertexCount();
    m_currentFrameStats.indexCount += mesh.getIndexCount();
}

void Renderer::drawLines(const Mesh& lineMesh,
                         Shader& shader,
                         const glm::mat4& modelMatrix,
                         const Camera& camera,
                         const glm::vec4& color)
{
    if (!lineMesh.isValid() || !shader.isValid()) {
        return;
    }

    shader.bind();
    shader.setMat4("uModel", modelMatrix);
    shader.setMat4("uView", camera.getViewMatrix());
    shader.setMat4("uProjection", camera.getProjectionMatrix());
    shader.setVec4("uColor", color);

    lineMesh.draw();

    ++m_currentFrameStats.drawCalls;
    m_currentFrameStats.vertexCount += lineMesh.getVertexCount();
    m_currentFrameStats.indexCount += lineMesh.getIndexCount();
}

void Renderer::endFrame()
{
    Shader::unbind();
    m_lastFrameStats = m_currentFrameStats;
}

} // namespace roomcraft::graphics
