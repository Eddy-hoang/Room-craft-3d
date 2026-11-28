#include "roomcraft/graphics/ShadowMap.hpp"

#include <glad/glad.h>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace roomcraft::graphics {

ShadowMap::~ShadowMap()
{
    destroy();
}

bool ShadowMap::create(int resolution)
{
    const int clamped = std::clamp(resolution, 256, 2048);
    if (isValid() && m_resolution == clamped) {
        return true;
    }

    unsigned int fbo = 0;
    unsigned int depthTex = 0;
    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &depthTex);

    glBindTexture(GL_TEXTURE_2D, depthTex);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_DEPTH_COMPONENT24,
                 clamped,
                 clamped,
                 0,
                 GL_DEPTH_COMPONENT,
                 GL_FLOAT,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float borderColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        glDeleteTextures(1, &depthTex);
        glDeleteFramebuffers(1, &fbo);
        return false;
    }

    destroy();
    m_fbo = fbo;
    m_depthTexture = depthTex;
    m_resolution = clamped;
    return true;
}

bool ShadowMap::resize(int resolution)
{
    return create(resolution);
}

void ShadowMap::destroy()
{
    if (m_depthTexture != 0) {
        glDeleteTextures(1, &m_depthTexture);
        m_depthTexture = 0;
    }
    if (m_fbo != 0) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
    m_resolution = 0;
}

void ShadowMap::bindForWriting() const
{
    if (!isValid()) {
        return;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_resolution, m_resolution);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMap::bindDepthTexture(unsigned int textureUnit) const
{
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
}

glm::mat4 ShadowMap::computeDirectionalLightSpaceMatrix(const glm::vec3& lightDirection,
                                                        const math::BoundingBox& sceneBounds)
{
    glm::vec3 dir = lightDirection;
    const float lenSq = glm::dot(dir, dir);
    if (!std::isfinite(lenSq) || lenSq < 1e-8f) {
        dir = glm::normalize(glm::vec3(-0.6f, -1.0f, -0.45f));
    } else {
        dir = glm::normalize(dir);
    }

    math::BoundingBox bounds = sceneBounds;
    if (!bounds.isValid()) {
        bounds.min = glm::vec3(-3.0f, 0.0f, -3.0f);
        bounds.max = glm::vec3(3.0f, 3.0f, 3.0f);
    }

    const glm::vec3 center = bounds.getCenter();
    const float radius = std::max(1.5f, glm::length(bounds.getHalfExtents()) + 0.5f);

    // Chọn vector Up không song song với hướng ánh sáng
    const glm::vec3 up = (std::fabs(dir.y) > 0.96f) ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 lightPos = center - dir * (radius * 2.2f);
    const glm::mat4 lightView = glm::lookAt(lightPos, center, up);

    // Chiếu 8 đỉnh của hộp bao cảnh sang không gian đèn để dựng hộp chiếu trực giao khít nhất
    const std::array<glm::vec3, 8> corners = {
        glm::vec3(bounds.min.x, bounds.min.y, bounds.min.z),
        glm::vec3(bounds.max.x, bounds.min.y, bounds.min.z),
        glm::vec3(bounds.min.x, bounds.max.y, bounds.min.z),
        glm::vec3(bounds.max.x, bounds.max.y, bounds.min.z),
        glm::vec3(bounds.min.x, bounds.min.y, bounds.max.z),
        glm::vec3(bounds.max.x, bounds.min.y, bounds.max.z),
        glm::vec3(bounds.min.x, bounds.max.y, bounds.max.z),
        glm::vec3(bounds.max.x, bounds.max.y, bounds.max.z)
    };

    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();

    for (const auto& c : corners) {
        const glm::vec3 ls = glm::vec3(lightView * glm::vec4(c, 1.0f));
        minX = std::min(minX, ls.x);
        maxX = std::max(maxX, ls.x);
        minY = std::min(minY, ls.y);
        maxY = std::max(maxY, ls.y);
        minZ = std::min(minZ, ls.z);
        maxZ = std::max(maxZ, ls.z);
    }

    constexpr float padXY = 0.35f;
    const float nearPlane = std::max(0.1f, -maxZ - 2.0f);
    const float farPlane = std::max(nearPlane + 1.0f, -minZ + 2.0f);
    const glm::mat4 lightProj = glm::ortho(minX - padXY, maxX + padXY,
                                           minY - padXY, maxY + padXY,
                                           nearPlane, farPlane);
    return lightProj * lightView;
}

} // namespace roomcraft::graphics
