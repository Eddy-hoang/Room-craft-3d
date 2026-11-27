#include "roomcraft/graphics/Renderer.hpp"

#include <glad/glad.h>

namespace roomcraft::graphics {

void Renderer::beginFrame(int viewportWidth,
                          int viewportHeight,
                          const glm::vec4& clearColor,
                          bool enableDepthTest)
{
    m_currentFrameStats = RenderStats{};
    m_wireframeEnabled = false;
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

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

void Renderer::setWireframeMode(bool enabled)
{
    m_wireframeEnabled = enabled;
    glPolygonMode(GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL);
}

void Renderer::drawMesh(const Mesh& mesh,
                        Shader& shader,
                        const scene::Transform& transform,
                        const Camera& camera,
                        const glm::vec4& color,
                        float faceContrast)
{
    drawMeshTextured(mesh, shader, transform.getModelMatrix(), camera, color, nullptr, faceContrast);
}

void Renderer::drawMeshWithMatrix(const Mesh& mesh,
                                  Shader& shader,
                                  const glm::mat4& modelMatrix,
                                  const Camera& camera,
                                  const glm::vec4& color,
                                  float faceContrast)
{
    drawMeshTextured(mesh, shader, modelMatrix, camera, color, nullptr, faceContrast);
}

void Renderer::drawMeshTextured(const Mesh& mesh,
                                Shader& shader,
                                const glm::mat4& modelMatrix,
                                const Camera& camera,
                                const glm::vec4& color,
                                const Texture* diffuseTexture,
                                float faceContrast)
{
    if (!mesh.isValid() || !shader.isValid()) {
        return;
    }

    shader.bind();
    shader.setMat4("uModel", modelMatrix);
    shader.setMat4("uView", camera.getViewMatrix());
    shader.setMat4("uProjection", camera.getProjectionMatrix());
    shader.setVec4("uColor", color);
    shader.setFloat("uFaceContrast", faceContrast);

    const bool useTexture = (diffuseTexture != nullptr && diffuseTexture->isValid());
    if (useTexture) {
        diffuseTexture->bind(0);
        shader.setInt("uDiffuseMap", 0);
        shader.setInt("uHasTexture", 1);
    } else {
        shader.setInt("uHasTexture", 0);
    }

    mesh.draw();

    if (useTexture) {
        Texture::unbind(0);
        shader.setInt("uHasTexture", 0);
    }

    ++m_currentFrameStats.drawCalls;
    m_currentFrameStats.vertexCount += mesh.getVertexCount();
    m_currentFrameStats.indexCount += mesh.getIndexCount();
    if (mesh.getTopology() == PrimitiveTopology::Triangles) {
        m_currentFrameStats.triangleCount += (mesh.getIndexCount() / 3);
    }
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
    if (m_wireframeEnabled) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        m_wireframeEnabled = false;
    }
    Shader::unbind();
    m_lastFrameStats = m_currentFrameStats;
}

} // namespace roomcraft::graphics
