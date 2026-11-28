#include "roomcraft/graphics/TextureManager.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>

namespace roomcraft::graphics {

TextureManager::~TextureManager()
{
    clear();
}

std::string TextureManager::makeCacheKey(const std::filesystem::path& filePath)
{
    std::error_code ec;
    const auto canonicalPath = std::filesystem::weakly_canonical(filePath, ec);
    const auto rawU8 = ec ? filePath.lexically_normal().generic_u8string() : canonicalPath.generic_u8string();
    std::string key(rawU8.begin(), rawU8.end());
#ifdef _WIN32
    // Windows filesystem không phân biệt hoa thường nên chuẩn hóa khóa cache về chữ thường
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
#endif
    return key;
}

std::shared_ptr<Texture> TextureManager::getOrLoadTexture(const std::filesystem::path& filePath,
                                                          bool* outUsedFallback)
{
    if (outUsedFallback != nullptr) {
        *outUsedFallback = false;
    }

    if (filePath.empty()) {
        if (outUsedFallback != nullptr) {
            *outUsedFallback = true;
        }
        return getFallbackTexture();
    }

    const std::string key = makeCacheKey(filePath);
    const auto it = m_cache.find(key);
    if (it != m_cache.end() && it->second && it->second->isValid()) {
        return it->second;
    }

    auto tex = std::make_shared<Texture>();
    std::string err;
    if (!tex->loadFromFile(filePath, &err)) {
        const auto u8 = filePath.u8string();
        std::cerr << "[RoomCraft3D][WARN] Texture fallback activated for '" << std::string(u8.begin(), u8.end())
                  << "' (" << err << ")" << std::endl;
        if (outUsedFallback != nullptr) {
            *outUsedFallback = true;
        }
        return getFallbackTexture();
    }

    m_cache.emplace(key, tex);
    return tex;
}

std::shared_ptr<Texture> TextureManager::getFallbackTexture()
{
    if (!m_fallbackTexture || !m_fallbackTexture->isValid()) {
        m_fallbackTexture = std::make_shared<Texture>();
        m_fallbackTexture->createFallbackChecker(64);
    }
    return m_fallbackTexture;
}

std::shared_ptr<Texture> TextureManager::getWhiteTexture()
{
    if (!m_whiteTexture || !m_whiteTexture->isValid()) {
        m_whiteTexture = std::make_shared<Texture>();
        m_whiteTexture->createSolidColor(255, 255, 255, 255);
    }
    return m_whiteTexture;
}

void TextureManager::clear()
{
    for (auto& [key, tex] : m_cache) {
        if (tex) {
            tex->destroy();
        }
    }
    m_cache.clear();

    if (m_fallbackTexture) {
        m_fallbackTexture->destroy();
        m_fallbackTexture.reset();
    }
    if (m_whiteTexture) {
        m_whiteTexture->destroy();
        m_whiteTexture.reset();
    }
}

} // namespace roomcraft::graphics
