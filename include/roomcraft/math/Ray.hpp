#pragma once

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cmath>

namespace roomcraft::math {

// Hình chữ nhật hiển thị của ảnh Viewport 3D bên trong Dear ImGui,
// hỗ trợ quy đổi tọa độ chuột màn hình -> viewport cục bộ -> framebuffer -> NDC (kể cả khi DPI/kích thước khác nhau).
struct ViewportRect {
    float minX = 0.0f;
    float minY = 0.0f;
    float maxX = 0.0f;
    float maxY = 0.0f;
    float framebufferWidth = 0.0f;
    float framebufferHeight = 0.0f;

    float getWidth() const { return maxX - minX; }
    float getHeight() const { return maxY - minY; }

    bool isValid() const
    {
        return std::isfinite(minX) && std::isfinite(minY) &&
               std::isfinite(maxX) && std::isfinite(maxY) &&
               getWidth() > 1e-3f && getHeight() > 1e-3f;
    }

    bool contains(float screenX, float screenY) const
    {
        return isValid() &&
               std::isfinite(screenX) && std::isfinite(screenY) &&
               screenX >= minX && screenX <= maxX &&
               screenY >= minY && screenY <= maxY;
    }

    // Chuyển tọa độ chuột cửa sổ (gốc trên-trái) sang tọa độ cục bộ trong ảnh viewport [0..width, 0..height]
    bool mapScreenToViewportLocal(float screenX, float screenY, glm::vec2& outLocal) const
    {
        if (!isValid() || !std::isfinite(screenX) || !std::isfinite(screenY)) {
            return false;
        }
        outLocal.x = screenX - minX;
        outLocal.y = screenY - minY;
        return true;
    }

    // Quy đổi sang tọa độ pixel thực tế của Framebuffer (xử lý chênh lệch display size vs framebuffer size / DPI)
    bool mapScreenToFramebuffer(float screenX, float screenY, glm::vec2& outFbPos) const
    {
        glm::vec2 local(0.0f);
        if (!mapScreenToViewportLocal(screenX, screenY, local)) {
            return false;
        }
        const float fbW = (framebufferWidth > 1e-3f) ? framebufferWidth : getWidth();
        const float fbH = (framebufferHeight > 1e-3f) ? framebufferHeight : getHeight();
        const float scaleX = fbW / getWidth();
        const float scaleY = fbH / getHeight();
        outFbPos.x = local.x * scaleX;
        outFbPos.y = local.y * scaleY;
        return true;
    }

    // Quy đổi sang Normalized Device Coordinates (NDC) của OpenGL:
    //   ndcX thuộc [-1, +1] (trái -> phải)
    //   ndcY thuộc [-1, +1] (dưới -> trên; lật trục Y từ tọa độ màn hình gốc trên-trái)
    bool mapScreenToNdc(float screenX, float screenY, glm::vec2& outNdc) const
    {
        glm::vec2 local(0.0f);
        if (!mapScreenToViewportLocal(screenX, screenY, local)) {
            return false;
        }
        const float u = local.x / getWidth();
        const float v = local.y / getHeight();
        outNdc.x = 2.0f * u - 1.0f;
        outNdc.y = 1.0f - 2.0f * v;
        return std::isfinite(outNdc.x) && std::isfinite(outNdc.y);
    }
};

// Tia 3D biểu diễn theo phương trình tham số: R(t) = origin + t * direction (với t >= 0).
struct Ray {
    glm::vec3 origin{0.0f, 0.0f, 0.0f};
    glm::vec3 direction{0.0f, 0.0f, -1.0f};

    bool isValid() const
    {
        const bool finiteOrig = std::isfinite(origin.x) && std::isfinite(origin.y) && std::isfinite(origin.z);
        const bool finiteDir = std::isfinite(direction.x) && std::isfinite(direction.y) && std::isfinite(direction.z);
        if (!finiteOrig || !finiteDir) {
            return false;
        }
        return glm::length(direction) > 1e-6f;
    }

    glm::vec3 at(float t) const
    {
        return origin + direction * t;
    }

    static bool create(const glm::vec3& origin, const glm::vec3& direction, Ray& outRay)
    {
        if (!std::isfinite(origin.x) || !std::isfinite(origin.y) || !std::isfinite(origin.z) ||
            !std::isfinite(direction.x) || !std::isfinite(direction.y) || !std::isfinite(direction.z)) {
            return false;
        }
        const float len = glm::length(direction);
        if (!std::isfinite(len) || len <= 1e-6f) {
            return false;
        }
        outRay.origin = origin;
        outRay.direction = direction / len;
        return true;
    }

    // Chuyển tọa độ chuột màn hình thành tia 3D trong không gian thế giới (World-space Ray).
    // Hoạt động thống nhất cho cả phép chiếu Phối cảnh (Perspective) và Trực giao (Orthographic):
    // - Giải chiếu điểm gần tại NDC (ndcX, ndcY, -1) và điểm xa tại NDC (ndcX, ndcY, +1) bằng (P * V)^-1.
    // - Thực hiện chia thuần nhất (homogeneous division / w) an toàn.
    // - Trong Orthographic, điểm xuất phát (origin = nearWorld) thay đổi theo con trỏ còn hướng tia song song.
    static bool screenToWorldRay(float screenX,
                                 float screenY,
                                 const ViewportRect& viewport,
                                 const glm::mat4& viewMatrix,
                                 const glm::mat4& projectionMatrix,
                                 Ray& outRay,
                                 bool requireInsideViewport = true)
    {
        if (!viewport.isValid()) {
            return false;
        }
        if (requireInsideViewport && !viewport.contains(screenX, screenY)) {
            return false;
        }

        glm::vec2 ndc(0.0f);
        if (!viewport.mapScreenToNdc(screenX, screenY, ndc)) {
            return false;
        }

        const glm::mat4 vp = projectionMatrix * viewMatrix;
        const float det = glm::determinant(vp);
        if (!std::isfinite(det) || std::fabs(det) <= 1e-12f) {
            return false;
        }

        const glm::mat4 invVp = glm::inverse(vp);
        const glm::vec4 nearClip(ndc.x, ndc.y, -1.0f, 1.0f);
        const glm::vec4 farClip(ndc.x, ndc.y, 1.0f, 1.0f);

        const glm::vec4 nearH = invVp * nearClip;
        const glm::vec4 farH = invVp * farClip;

        if (!std::isfinite(nearH.w) || !std::isfinite(farH.w) ||
            std::fabs(nearH.w) <= 1e-6f || std::fabs(farH.w) <= 1e-6f) {
            return false;
        }

        const glm::vec3 nearWorld = glm::vec3(nearH) / nearH.w;
        const glm::vec3 farWorld = glm::vec3(farH) / farH.w;
        return Ray::create(nearWorld, farWorld - nearWorld, outRay);
    }
};

} // namespace roomcraft::math
