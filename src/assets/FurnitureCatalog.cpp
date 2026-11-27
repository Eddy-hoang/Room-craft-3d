#include "roomcraft/assets/FurnitureCatalog.hpp"

#include <algorithm>
#include <cctype>

namespace roomcraft::assets {

namespace {

std::string toLowerAscii(const std::string& input)
{
    std::string out = input;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

} // namespace

FurnitureCatalog::FurnitureCatalog()
{
    registerDefaultEntries();
}

void FurnitureCatalog::registerDefaultEntries()
{
    m_entries.clear();

    // ==================== 6 MẪU NỘI THẤT PROCEDURAL (M2 - C++) ====================
    registerEntry({
        "procedural-desk",
        "Procedural Desk",
        AssetCategory::Tables,
        AssetSourceType::Procedural,
        "",
        1.0f,
        "Right-handed Y-up (meters, bottom-center pivot)",
        {1.40f, 0.75f, 0.70f},
        "",
        "RoomCraft 3D (C++ Geometry Engine M2)",
        "MIT",
        "src/geometry/FurnitureGenerator.cpp",
        geometry::FurnitureType::Desk
    });

    registerEntry({
        "procedural-chair",
        "Procedural Chair",
        AssetCategory::Seating,
        AssetSourceType::Procedural,
        "",
        1.0f,
        "Right-handed Y-up (meters, bottom-center pivot)",
        {0.46f, 0.89f, 0.46f},
        "",
        "RoomCraft 3D (C++ Geometry Engine M2)",
        "MIT",
        "src/geometry/FurnitureGenerator.cpp",
        geometry::FurnitureType::Chair
    });

    registerEntry({
        "procedural-bookshelf",
        "Procedural Bookshelf",
        AssetCategory::Storage,
        AssetSourceType::Procedural,
        "",
        1.0f,
        "Right-handed Y-up (meters, bottom-center pivot)",
        {0.90f, 1.80f, 0.32f},
        "",
        "RoomCraft 3D (C++ Geometry Engine M2)",
        "MIT",
        "src/geometry/FurnitureGenerator.cpp",
        geometry::FurnitureType::Bookshelf
    });

    registerEntry({
        "procedural-desk-lamp",
        "Procedural Desk Lamp",
        AssetCategory::Lighting,
        AssetSourceType::Procedural,
        "",
        1.0f,
        "Right-handed Y-up (meters, bottom-center pivot)",
        {0.27f, 0.51f, 0.27f},
        "",
        "RoomCraft 3D (C++ Geometry Engine M2)",
        "MIT",
        "src/geometry/FurnitureGenerator.cpp",
        geometry::FurnitureType::DeskLamp
    });

    registerEntry({
        "procedural-vase",
        "Procedural Ceramic Vase",
        AssetCategory::Decor,
        AssetSourceType::Procedural,
        "",
        1.0f,
        "Right-handed Y-up (meters, bottom-center pivot)",
        {0.20f, 0.40f, 0.20f},
        "",
        "RoomCraft 3D (C++ Geometry Engine M2)",
        "MIT",
        "src/geometry/FurnitureGenerator.cpp",
        geometry::FurnitureType::Vase
    });

    registerEntry({
        "procedural-curved-chair",
        "Procedural Curved Chair",
        AssetCategory::Seating,
        AssetSourceType::Procedural,
        "",
        1.0f,
        "Right-handed Y-up (meters, bottom-center pivot)",
        {0.50f, 0.91f, 0.58f},
        "",
        "RoomCraft 3D (C++ Geometry Engine M2)",
        "MIT",
        "src/geometry/FurnitureGenerator.cpp",
        geometry::FurnitureType::CurvedChair
    });

    // ==================== 8 MẪU NỘI THẤT BLENDER (M3 - OBJ/MTL) ====================
    registerEntry({
        "blender-modern-sofa",
        "Modern Sofa",
        AssetCategory::Seating,
        AssetSourceType::Imported,
        "assets/models/sofa/modern_sofa.obj",
        1.0f,
        "Blender OBJ export (Up Y, Forward -Z, meters)",
        {2.10f, 0.85f, 0.92f},
        "",
        "RoomCraft 3D (Blender bpy script)",
        "MIT",
        "blender/scripts/create_sofa.py",
        std::nullopt
    });

    registerEntry({
        "blender-modern-bed",
        "Modern Bed",
        AssetCategory::Beds,
        AssetSourceType::Imported,
        "assets/models/bed/modern_bed.obj",
        1.0f,
        "Blender OBJ export (Up Y, Forward -Z, meters)",
        {1.75f, 1.05f, 2.15f},
        "",
        "RoomCraft 3D (Blender bpy script)",
        "MIT",
        "blender/scripts/create_bed.py",
        std::nullopt
    });

    registerEntry({
        "blender-gaming-chair",
        "Gaming Chair",
        AssetCategory::Seating,
        AssetSourceType::Imported,
        "assets/models/gaming_chair/gaming_chair.obj",
        1.0f,
        "Blender OBJ export (Up Y, Forward -Z, meters)",
        {0.68f, 1.26f, 0.68f},
        "",
        "RoomCraft 3D (Blender bpy script)",
        "MIT",
        "blender/scripts/create_gaming_chair.py",
        std::nullopt
    });

    registerEntry({
        "blender-computer-setup",
        "Computer Setup",
        AssetCategory::Electronics,
        AssetSourceType::Imported,
        "assets/models/computer/computer_setup.obj",
        1.0f,
        "Blender OBJ export (Up Y, Forward -Z, meters)",
        {0.92f, 0.48f, 0.48f},
        "",
        "RoomCraft 3D (Blender bpy script)",
        "MIT",
        "blender/scripts/create_computer.py",
        std::nullopt
    });

    registerEntry({
        "blender-indoor-plant",
        "Indoor Plant",
        AssetCategory::Decor,
        AssetSourceType::Imported,
        "assets/models/plant/indoor_plant.obj",
        1.0f,
        "Blender OBJ export (Up Y, Forward -Z, meters)",
        {0.58f, 1.15f, 0.58f},
        "",
        "RoomCraft 3D (Blender bpy script)",
        "MIT",
        "blender/scripts/create_plant.py",
        std::nullopt
    });

    registerEntry({
        "blender-floor-lamp",
        "Floor Lamp",
        AssetCategory::Lighting,
        AssetSourceType::Imported,
        "assets/models/floor_lamp/floor_lamp.obj",
        1.0f,
        "Blender OBJ export (Up Y, Forward -Z, meters)",
        {0.48f, 1.62f, 0.48f},
        "",
        "RoomCraft 3D (Blender bpy script)",
        "MIT",
        "blender/scripts/create_floor_lamp.py",
        std::nullopt
    });

    registerEntry({
        "blender-wardrobe",
        "Wardrobe",
        AssetCategory::Storage,
        AssetSourceType::Imported,
        "assets/models/wardrobe/wardrobe.obj",
        1.0f,
        "Blender OBJ export (Up Y, Forward -Z, meters)",
        {1.40f, 2.05f, 0.60f},
        "",
        "RoomCraft 3D (Blender bpy script)",
        "MIT",
        "blender/scripts/create_wardrobe.py",
        std::nullopt
    });

    registerEntry({
        "blender-carpet",
        "Minimalist Carpet",
        AssetCategory::Decor,
        AssetSourceType::Imported,
        "assets/models/carpet/minimalist_carpet.obj",
        1.0f,
        "Blender OBJ export (Up Y, Forward -Z, meters)",
        {2.20f, 0.02f, 1.50f},
        "",
        "RoomCraft 3D (Blender bpy script)",
        "MIT",
        "blender/scripts/create_carpet.py",
        std::nullopt
    });
}

void FurnitureCatalog::registerEntry(CatalogEntry entry)
{
    for (auto& existing : m_entries) {
        if (existing.assetId == entry.assetId) {
            existing = std::move(entry);
            return;
        }
    }
    m_entries.push_back(std::move(entry));
}

const CatalogEntry* FurnitureCatalog::findById(const std::string& assetId) const
{
    for (const auto& entry : m_entries) {
        if (entry.assetId == assetId) {
            return &entry;
        }
    }
    return nullptr;
}

std::vector<const CatalogEntry*> FurnitureCatalog::filter(std::optional<AssetSourceType> sourceFilter,
                                                          std::optional<AssetCategory> categoryFilter,
                                                          const std::string& searchQuery) const
{
    const std::string needle = toLowerAscii(searchQuery);
    std::vector<const CatalogEntry*> results;
    results.reserve(m_entries.size());

    for (const auto& entry : m_entries) {
        if (sourceFilter.has_value() && entry.sourceType != *sourceFilter) {
            continue;
        }
        if (categoryFilter.has_value() && entry.category != *categoryFilter) {
            continue;
        }
        if (!needle.empty()) {
            const std::string hayName = toLowerAscii(entry.displayName);
            const std::string hayId = toLowerAscii(entry.assetId);
            const std::string hayCat = toLowerAscii(assetCategoryToString(entry.category));
            if (hayName.find(needle) == std::string::npos &&
                hayId.find(needle) == std::string::npos &&
                hayCat.find(needle) == std::string::npos) {
                continue;
            }
        }
        results.push_back(&entry);
    }

    return results;
}

} // namespace roomcraft::assets
