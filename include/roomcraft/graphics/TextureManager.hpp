#pragma once

#include "roomcraft/graphics/Texture.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace roomcraft::graphics {

// Lớp TextureManager quản lý bộ đệm (cache) OpenGL Texture theo đường dẫn chuẩn hóa,
// tránh nạp trùng lặp cùng một ảnh nhiều lần và cung cấp fallback texture khi ảnh thiếu/lỗi.
class TextureManager {
public:
    TextureManager() = default;
    ~TextureManager();

    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    // Nạp hoặc trả về bản cache của texture từ filePath; nếu lỗi trả về fallback texture và ghi cảnh báo
    std::shared_ptr<Texture> getOrLoadTexture(const std::filesystem::path& filePath,
                                              bool* outUsedFallback = nullptr);

    // Texture dự phòng (checker) và texture trắng 1x1
    std::shared_ptr<Texture> getFallbackTexture();
    std::shared_ptr<Texture> getWhiteTexture();

    std::size_t getCachedTextureCount() const { return m_cache.size(); }

    // Giải phóng toàn bộ texture GPU trước khi OpenGL context bị hủy
    void clear();

private:
    static std::string makeCacheKey(const std::filesystem::path& filePath);

private:
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_cache;
    std::shared_ptr<Texture> m_fallbackTexture;
    std::shared_ptr<Texture> m_whiteTexture;
};

} // namespace roomcraft::graphics
