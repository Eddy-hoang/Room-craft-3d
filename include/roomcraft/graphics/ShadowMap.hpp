#pragma once

#include "roomcraft/math/BoundingBox.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace roomcraft::graphics {

// Depth-only Shadow Map cho 1 nguồn sáng Directional Light (OpenGL 3.3 Core — M9.4).
// Hỗ trợ cấu hình độ phân giải (512 / 1024 / 2048), tự căn khung trực giao bao khít phòng
// và giải phóng tài nguyên GPU an toàn (RAII).
class ShadowMap {
public:
    ShadowMap() = default;
    ~ShadowMap();

    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    bool create(int resolution = 1024);
    bool resize(int resolution);
    void destroy();

    bool isValid() const { return m_fbo != 0 && m_depthTexture != 0 && m_resolution > 0; }
    int getResolution() const { return m_resolution; }
    unsigned int getDepthTextureId() const { return m_depthTexture; }

    void bindForWriting() const;
    static void unbind();
    void bindDepthTexture(unsigned int textureUnit) const;

    // Thông tin frustum trực giao dùng cho normal-offset bias chính xác theo kích thước texel thế giới
    struct FrustumInfo {
        glm::mat4 lightSpaceMatrix{1.0f};
        float texelWorldSize = 0.01f;
        float depthSpan = 10.0f;
    };

    // Tính ma trận biến đổi Light Space (Orthographic View * Projection) có texel-snapping ổn định
    static FrustumInfo computeDirectionalLightFrustum(const glm::vec3& lightDirection,
                                                      const math::BoundingBox& sceneBounds,
                                                      int shadowResolution = 1024);

    // Giữ nguyên API cũ cho tương thích ngược
    static glm::mat4 computeDirectionalLightSpaceMatrix(const glm::vec3& lightDirection,
                                                        const math::BoundingBox& sceneBounds);

private:
    unsigned int m_fbo = 0;
    unsigned int m_depthTexture = 0;
    int m_resolution = 0;
};

} // namespace roomcraft::graphics
