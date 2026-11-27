#include "roomcraft/graphics/Framebuffer.hpp"

#include <glad/glad.h>

#include <iostream>
#include <utility>

namespace roomcraft::graphics {

Framebuffer::~Framebuffer()
{
    destroy();
}

Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    : m_fbo(std::exchange(other.m_fbo, 0))
    , m_colorTexture(std::exchange(other.m_colorTexture, 0))
    , m_depthStencilRbo(std::exchange(other.m_depthStencilRbo, 0))
    , m_width(std::exchange(other.m_width, 0))
    , m_height(std::exchange(other.m_height, 0))
    , m_complete(std::exchange(other.m_complete, false))
{
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept
{
    if (this != &other) {
        destroy();
        m_fbo = std::exchange(other.m_fbo, 0);
        m_colorTexture = std::exchange(other.m_colorTexture, 0);
        m_depthStencilRbo = std::exchange(other.m_depthStencilRbo, 0);
        m_width = std::exchange(other.m_width, 0);
        m_height = std::exchange(other.m_height, 0);
        m_complete = std::exchange(other.m_complete, false);
    }
    return *this;
}

bool Framebuffer::create(int width, int height)
{
    if (width <= 0 || height <= 0) {
        std::cerr << "[RoomCraft3D][ERROR] Framebuffer dimensions must be positive ("
                  << width << "x" << height << ")." << std::endl;
        return false;
    }

    return allocateAttachments(width, height);
}

bool Framebuffer::resize(int width, int height)
{
    // Xử lý an toàn khi cửa sổ bị thu nhỏ (width == 0 hoặc height == 0): giữ nguyên FBO hợp lệ hiện có
    if (width <= 0 || height <= 0) {
        return isComplete();
    }

    // Không tạo lại tài nguyên GPU nếu kích thước không thay đổi
    if (isComplete() && width == m_width && height == m_height) {
        return true;
    }

    return allocateAttachments(width, height);
}

bool Framebuffer::allocateAttachments(int width, int height)
{
    destroy();

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // 1. Color Attachment (Texture RGBA8 để hiển thị vào ImGui viewport hoặc đọc pixel kiểm thử)
    glGenTextures(1, &m_colorTexture);
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // 2. Depth + Stencil Attachment (Renderbuffer 24-bit depth + 8-bit stencil cho Depth Testing)
    glGenRenderbuffers(1, &m_depthStencilRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthStencilRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthStencilRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // 3. Kiểm tra tính hoàn thiện của Framebuffer (Framebuffer Completeness)
    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[RoomCraft3D][ERROR] Framebuffer is incomplete! Status: 0x"
                  << std::hex << status << std::dec << std::endl;
        destroy();
        return false;
    }

    m_width = width;
    m_height = height;
    m_complete = true;
    return true;
}

void Framebuffer::bind() const
{
    if (!isComplete()) {
        return;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);
}

void Framebuffer::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::destroy()
{
    if (m_depthStencilRbo != 0) {
        glDeleteRenderbuffers(1, &m_depthStencilRbo);
        m_depthStencilRbo = 0;
    }
    if (m_colorTexture != 0) {
        glDeleteTextures(1, &m_colorTexture);
        m_colorTexture = 0;
    }
    if (m_fbo != 0) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
    m_width = 0;
    m_height = 0;
    m_complete = false;
}

} // namespace roomcraft::graphics
