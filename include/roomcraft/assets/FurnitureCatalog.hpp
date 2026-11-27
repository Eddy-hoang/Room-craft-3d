#pragma once

#include "roomcraft/assets/AssetData.hpp"
#include "roomcraft/geometry/FurnitureGenerator.hpp"

#include <glm/vec3.hpp>

#include <optional>
#include <string>
#include <vector>

namespace roomcraft::assets {

// Bản ghi siêu dữ liệu (metadata) cho một tài sản nội thất trong danh mục thống nhất
struct CatalogEntry {
    std::string assetId;
    std::string displayName;
    AssetCategory category = AssetCategory::Decor;
    AssetSourceType sourceType = AssetSourceType::Imported;
    std::string relativeModelPath;
    float unitScale = 1.0f;
    std::string coordinateConvention = "Right-handed Y-up (Forward -Z, meters)";
    glm::vec3 defaultDimensions{1.0f, 1.0f, 1.0f};
    std::string thumbnailPath;
    std::string author = "RoomCraft 3D";
    std::string license = "MIT";
    std::string sourceReference;
    std::optional<geometry::FurnitureType> proceduralType;
};

// Danh mục nội thất thống nhất quản lý cả 6 mẫu Procedural (M2) và 8 mẫu Blender (M3)
class FurnitureCatalog {
public:
    FurnitureCatalog();

    // Nạp danh mục mặc định gồm 6 procedural assets + 8 Blender assets
    void registerDefaultEntries();
    void registerEntry(CatalogEntry entry);

    const std::vector<CatalogEntry>& getAllEntries() const { return m_entries; }
    const CatalogEntry* findById(const std::string& assetId) const;

    // Lọc danh mục theo nguồn (All/Procedural/Imported), phân loại và từ khóa tìm kiếm không phân biệt hoa thường
    std::vector<const CatalogEntry*> filter(std::optional<AssetSourceType> sourceFilter,
                                            std::optional<AssetCategory> categoryFilter,
                                            const std::string& searchQuery) const;

private:
    std::vector<CatalogEntry> m_entries;
};

} // namespace roomcraft::assets
