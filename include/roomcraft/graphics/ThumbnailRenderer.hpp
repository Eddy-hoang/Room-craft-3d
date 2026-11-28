#pragma once

#include "roomcraft/assets/AssetManager.hpp"
#include "roomcraft/graphics/Shader.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace roomcraft::graphics {

struct ThumbnailEntry {
    unsigned int textureId = 0;
    std::weak_ptr<assets::GpuAsset> sourceAsset;
    int width = 0;
    int height = 0;
    double generationMs = 0.0;
};

// Bộ sinh và lưu trữ ảnh xem trước (Thumbnail 256x256) bằng chính OpenGL 3.3 Core Framebuffer.
// - Render đúng mô hình thực tế (cả 6 Procedural + 8 Blender) theo góc nhìn 3/4 nhất quán.
// - Không vẽ Grid, Axes hay BoundingBox trong ảnh thumbnail.
// - Tự động lưu và khôi phục hoàn toàn OpenGL state (FBO, viewport, depth test, polygon mode)
//   để không làm ảnh hưởng tới Room Editor.
// - Cache theo assetId + weak_ptr của GpuAsset hiện tại; tự invalidate khi reloadAsset().
class ThumbnailRenderer {
public:
    static constexpr int kDefaultSize = 256;

    ThumbnailRenderer() = default;
    ~ThumbnailRenderer();

    ThumbnailRenderer(const ThumbnailRenderer&) = delete;
    ThumbnailRenderer& operator=(const ThumbnailRenderer&) = delete;

    // Trả về OpenGL Texture ID từ cache hoặc sinh mới đúng một lần
    unsigned int getOrCreateThumbnail(const std::string& assetId,
                                      assets::AssetManager& assetManager,
                                      Shader& meshShader,
                                      int size = kDefaultSize);

    // Xóa cache của một asset (gọi khi reloadAsset hoặc khi muốn ép sinh lại)
    void invalidate(const std::string& assetId);

    // Giải phóng toàn bộ GPU textures + FBO/RBO trước khi hủy OpenGL context
    void destroyAll();

    std::size_t getCachedCount() const { return m_cache.size(); }
    std::uint64_t getTotalGenerations() const { return m_totalGenerations; }
    double getTotalGenerationTimeMs() const { return m_totalGenerationMs; }

private:
    bool ensureFramebufferResources(int size);
    bool renderAssetIntoTexture(const assets::GpuAsset& asset,
                                Shader& meshShader,
                                int size,
                                unsigned int& outTextureId,
                                double& outGenerationMs);

private:
    unsigned int m_fbo = 0;
    unsigned int m_depthRbo = 0;
    int m_rboSize = 0;

    std::unordered_map<std::string, ThumbnailEntry> m_cache;
    std::uint64_t m_totalGenerations = 0;
    double m_totalGenerationMs = 0.0;
};

} // namespace roomcraft::graphics
