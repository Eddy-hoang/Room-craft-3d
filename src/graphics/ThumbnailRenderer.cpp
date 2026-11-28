#include "roomcraft/graphics/ThumbnailRenderer.hpp"

#include "roomcraft/graphics/Camera.hpp"
#include "roomcraft/graphics/Renderer.hpp"

#include <glad/glad.h>
#include <glm/mat4x4.hpp>

#include <chrono>

namespace roomcraft::graphics {

ThumbnailRenderer::~ThumbnailRenderer()
{
    destroyAll();
}

bool ThumbnailRenderer::ensureFramebufferResources(int size)
{
    if (size <= 0) {
        return false;
    }

    if (m_fbo == 0) {
        glGenFramebuffers(1, &m_fbo);
    }
    if (m_depthRbo == 0 || m_rboSize != size) {
        if (m_depthRbo == 0) {
            glGenRenderbuffers(1, &m_depthRbo);
        }
        glBindRenderbuffer(GL_RENDERBUFFER, m_depthRbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, size, size);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        m_rboSize = size;
    }

    return m_fbo != 0 && m_depthRbo != 0;
}

bool ThumbnailRenderer::renderAssetIntoTexture(const assets::GpuAsset& asset,
                                               Shader& meshShader,
                                               int size,
                                               unsigned int& outTextureId,
                                               double& outGenerationMs)
{
    if (!meshShader.isValid() || asset.subMeshes.empty()) {
        return false;
    }

    const auto t0 = std::chrono::steady_clock::now();

    // Lưu toàn bộ OpenGL state hiện tại để khôi phục chính xác sau khi vẽ thumbnail
    GLint prevDrawFbo = 0;
    GLint prevReadFbo = 0;
    GLint prevViewport[4] = {0, 0, 0, 0};
    GLint prevPolygonMode[2] = {GL_FILL, GL_FILL};
    const GLboolean prevDepthTest = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean prevScissorTest = glIsEnabled(GL_SCISSOR_TEST);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevDrawFbo);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevReadFbo);
    glGetIntegerv(GL_VIEWPORT, prevViewport);
    glGetIntegerv(GL_POLYGON_MODE, prevPolygonMode);
    if (prevScissorTest) {
        glDisable(GL_SCISSOR_TEST);
    }

    if (!ensureFramebufferResources(size)) {
        return false;
    }

    unsigned int texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthRbo);

    const bool complete = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    if (complete) {
        // Camera studio riêng biệt (góc nhìn 3/4 nhất quán, auto-fit đúng tỷ lệ thực của nội thất)
        Camera thumbCam;
        thumbCam.setProjectionMode(ProjectionMode::Perspective);
        thumbCam.setAspectRatio(1.0f);
        thumbCam.lookAt(glm::vec3(2.6f, 1.85f, 2.8f),
                        glm::vec3(0.0f, 0.45f, 0.0f),
                        glm::vec3(0.0f, 1.0f, 0.0f));
        thumbCam.fitBoundingBox(asset.cpuData.localBounds, 1.32f);

        // Dùng Renderer cục bộ để không ghi đè RenderStats của Room Editor
        Renderer thumbRenderer;
        const glm::vec4 neutralStudioBg(0.165f, 0.188f, 0.232f, 1.0f); // #2A303B
        thumbRenderer.beginFrame(size, size, neutralStudioBg, true);
        thumbRenderer.applyLighting(meshShader, thumbCam, LightingSettings::preset(LightingPreset::NeutralStudio));

        for (const auto& sub : asset.subMeshes) {
            const glm::mat4 worldMat = sub.localTransform.getModelMatrix();
            glm::vec4 baseColor(0.80f, 0.80f, 0.82f, 1.0f);
            if (sub.materialIndex < asset.cpuData.materials.size()) {
                baseColor = asset.cpuData.materials[sub.materialIndex].baseColor;
            }
            const Texture* diffuseTex = nullptr;
            if (sub.materialIndex < asset.materialDiffuseTextures.size() &&
                asset.materialDiffuseTextures[sub.materialIndex] != nullptr) {
                diffuseTex = asset.materialDiffuseTextures[sub.materialIndex].get();
            }
            thumbRenderer.drawMeshTextured(sub.mesh, meshShader, worldMat, thumbCam, baseColor, diffuseTex, 1.0f,
                                          &asset.cpuData.materials[sub.materialIndex]);
        }

        thumbRenderer.endFrame();
    }

    // Tháo texture trước khi trả FBO về trạng thái trước đó
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(prevDrawFbo));
    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(prevReadFbo));
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    glPolygonMode(GL_FRONT_AND_BACK, static_cast<GLenum>(prevPolygonMode[0]));
    if (prevDepthTest) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    if (prevScissorTest) {
        glEnable(GL_SCISSOR_TEST);
    }

    if (!complete) {
        glDeleteTextures(1, &texId);
        return false;
    }

    const auto t1 = std::chrono::steady_clock::now();
    outTextureId = texId;
    outGenerationMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return true;
}

unsigned int ThumbnailRenderer::getOrCreateThumbnail(const std::string& assetId,
                                                     assets::AssetManager& assetManager,
                                                     Shader& meshShader,
                                                     int size)
{
    auto gpuAsset = assetManager.getOrLoadAsset(assetId, nullptr);
    if (!gpuAsset) {
        return 0;
    }

    const auto it = m_cache.find(assetId);
    if (it != m_cache.end() &&
        it->second.textureId != 0 &&
        it->second.sourceAsset.lock() == gpuAsset &&
        it->second.width == size &&
        it->second.height == size) {
        return it->second.textureId;
    }

    unsigned int texId = 0;
    double genMs = 0.0;
    if (!renderAssetIntoTexture(*gpuAsset, meshShader, size, texId, genMs)) {
        return 0;
    }

    ThumbnailEntry entry;
    entry.textureId = texId;
    entry.sourceAsset = gpuAsset;
    if (it != m_cache.end()) glDeleteTextures(1, &it->second.textureId);
    entry.width = size;
    entry.height = size;
    entry.generationMs = genMs;
    m_cache[assetId] = entry;
    ++m_totalGenerations;
    m_totalGenerationMs += genMs;
    return texId;
}

void ThumbnailRenderer::invalidate(const std::string& assetId)
{
    const auto it = m_cache.find(assetId);
    if (it != m_cache.end()) {
        if (it->second.textureId != 0) {
            glDeleteTextures(1, &it->second.textureId);
        }
        m_cache.erase(it);
    }
}

void ThumbnailRenderer::destroyAll()
{
    for (auto& kv : m_cache) {
        if (kv.second.textureId != 0) {
            glDeleteTextures(1, &kv.second.textureId);
        }
    }
    m_cache.clear();

    if (m_depthRbo != 0) {
        glDeleteRenderbuffers(1, &m_depthRbo);
        m_depthRbo = 0;
    }
    if (m_fbo != 0) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
    m_rboSize = 0;
}

} // namespace roomcraft::graphics
