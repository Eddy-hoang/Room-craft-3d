#pragma once

#include "roomcraft/assets/AssetData.hpp"
#include "roomcraft/assets/FurnitureCatalog.hpp"
#include "roomcraft/graphics/Mesh.hpp"
#include "roomcraft/graphics/Texture.hpp"
#include "roomcraft/graphics/TextureManager.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace roomcraft::assets {

// SubMesh đã upload lên GPU (tách biệt với SubMeshData trên CPU)
struct GpuSubMesh {
    std::string name;
    graphics::Mesh mesh;
    std::size_t materialIndex = 0;
    scene::Transform localTransform;
};

// Tài nguyên Asset hoàn chỉnh trên GPU được chia sẻ (shared_ptr) giữa các lượt sử dụng
// để không sao chép VAO/VBO/EBO hoặc Texture trùng lặp
struct GpuAsset {
    AssetData cpuData;
    std::vector<GpuSubMesh> subMeshes;
    std::vector<std::shared_ptr<graphics::Texture>> materialDiffuseTextures;
    graphics::Mesh boundingBoxLines;
    graphics::Mesh vertexNormalLines;
    bool usedFallbackTexture = false;

    void destroyGpuResources()
    {
        for (auto& sub : subMeshes) {
            sub.mesh.destroy();
        }
        subMeshes.clear();
        materialDiffuseTextures.clear();
        boundingBoxLines.destroy();
        vertexNormalLines.destroy();
    }
};

// Lớp AssetManager quản lý vòng đời, cache CPU/GPU và transactional reload theo stable Asset ID
class AssetManager {
public:
    AssetManager() = default;
    ~AssetManager();

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    // Thiết lập các thư mục gốc để tìm đường dẫn tài nguyên tương đối (thư mục exe + thư mục cwd)
    void setSearchRoots(const std::filesystem::path& executableDir,
                        const std::filesystem::path& workingDir);

    FurnitureCatalog& getCatalog() { return m_catalog; }
    const FurnitureCatalog& getCatalog() const { return m_catalog; }
    graphics::TextureManager& getTextureManager() { return m_textureManager; }
    const graphics::TextureManager& getTextureManager() const { return m_textureManager; }

    // Nạp hoặc trả về bản cache GPU của asset theo stable assetId
    std::shared_ptr<GpuAsset> getOrLoadAsset(const std::string& assetId, std::string* outError = nullptr);

    // Tải lại asset theo mẫu transactional: chỉ thay thế cache khi load + validate + upload GPU đều thành công
    bool reloadAsset(const std::string& assetId, std::string* outError = nullptr);

    // Nạp trực tiếp một file OBJ tùy ý theo stable assetId (dùng cho kiểm thử fixture)
    std::shared_ptr<GpuAsset> loadCustomImportedAsset(const std::string& assetId,
                                                      const std::filesystem::path& filePath,
                                                      float unitScale = 1.0f,
                                                      std::string* outError = nullptr);

    std::size_t getCachedAssetCount() const { return m_assetCache.size(); }

    // Giải phóng toàn bộ tài nguyên GPU (Mesh + Texture) trước khi OpenGL context bị hủy
    void clear();

private:
    std::filesystem::path resolveRelativePath(const std::filesystem::path& relPath) const;
    bool buildCpuAssetFromEntry(const CatalogEntry& entry, AssetData& outData, std::string* outError) const;
    std::shared_ptr<GpuAsset> uploadCpuAssetToGpu(AssetData cpuData, std::string* outError);

private:
    std::filesystem::path m_executableDir;
    std::filesystem::path m_workingDir;
    FurnitureCatalog m_catalog;
    graphics::TextureManager m_textureManager;
    std::unordered_map<std::string, std::shared_ptr<GpuAsset>> m_assetCache;
};

} // namespace roomcraft::assets
