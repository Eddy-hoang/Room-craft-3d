#pragma once

namespace roomcraft::graphics {

// Lớp Framebuffer (FBO) hỗ trợ render off-screen vào texture màu (GL_RGBA8)
// kết hợp depth-stencil renderbuffer (GL_DEPTH24_STENCIL8) theo mô hình RAII.
class Framebuffer {
public:
    Framebuffer() = default;
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    Framebuffer(Framebuffer&& other) noexcept;
    Framebuffer& operator=(Framebuffer&& other) noexcept;

    // Tạo framebuffer với kích thước ban đầu (> 0)
    bool create(int width, int height);

    // Cập nhật kích thước nếu thay đổi; bỏ qua an toàn khi width <= 0 hoặc height <= 0 (cửa sổ thu nhỏ)
    // và không cấp phát lại nếu kích thước giữ nguyên
    bool resize(int width, int height);

    // Kích hoạt/hủy kích hoạt framebuffer để render off-screen
    void bind() const;
    int bindMultisampled(int requestedSamples);
    void resolveMultisampled() const;
    static void unbind();

    // Giải phóng tường minh trước khi OpenGL context bị hủy
    void destroy();

    bool isComplete() const { return m_complete && m_fbo != 0; }
    unsigned int getFboId() const { return m_fbo; }
    unsigned int getColorTextureId() const { return m_colorTexture; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    bool allocateAttachments(int width, int height);

private:
    void releaseMultisample();
    unsigned int m_msFbo = 0;
    unsigned int m_msColor = 0;
    unsigned int m_msDepth = 0;
    int m_samples = 1;
    int m_requestedSamples = 1;
    unsigned int m_fbo = 0;
    unsigned int m_colorTexture = 0;
    unsigned int m_depthStencilRbo = 0;
    int m_width = 0;
    int m_height = 0;
    bool m_complete = false;
};

} // namespace roomcraft::graphics
