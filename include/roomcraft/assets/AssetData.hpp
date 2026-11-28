#pragma once

#include "roomcraft/geometry/FurnitureGenerator.hpp"
#include "roomcraft/geometry/MeshData.hpp"
#include "roomcraft/graphics/Material.hpp"
#include "roomcraft/math/BoundingBox.hpp"
#include "roomcraft/scene/Transform.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace roomcraft::assets {

// Nguồn gốc tạo mô hình trong chiến lược Hybrid Modeling
enum class AssetSourceType {
    Procedural, // Tự sinh bằng thuật toán C++ (Geometry Engine M2)
    Imported    // Nhập từ Blender (OBJ/MTL) qua Assimp (M3)
};

// Danh mục nội thất dùng cho bộ lọc trong Furniture Asset Browser
enum class AssetCategory {
    Seating,
    Tables,
    Storage,
    Lighting,
    Beds,
    Electronics,
    Decor
};

inline const char* assetSourceTypeToString(AssetSourceType type)
{
    return (type == AssetSourceType::Procedural) ? "Procedural (C++)" : "Blender / OBJ";
}

inline const char* assetCategoryToString(AssetCategory category)
{
    switch (category) {
        case AssetCategory::Seating:     return "Seating";
        case AssetCategory::Tables:      return "Tables";
        case AssetCategory::Storage:     return "Storage";
        case AssetCategory::Lighting:    return "Lighting";
        case AssetCategory::Beds:        return "Beds";
        case AssetCategory::Electronics: return "Electronics";
        case AssetCategory::Decor:       return "Decor";
        default:                         return "Other";
    }
}

// Một thành phần lưới con của AssetData trên CPU (không chứa GPU handle)
struct SubMeshData {
    std::string name;
    geometry::MeshData meshData;
    std::size_t materialIndex = 0;
    scene::Transform localTransform;
};

// Cấu trúc dữ liệu thống nhất (Unified Asset Data) trên CPU cho cả nội thất Procedural (M2)
// và nội thất nhập từ Blender/Assimp (M3). Tuyệt đối không chứa OpenGL handles.
struct AssetData {
    std::string assetId;
    std::string displayName;
    AssetSourceType sourceType = AssetSourceType::Imported;
    AssetCategory category = AssetCategory::Decor;

    std::vector<SubMeshData> subMeshes;
    std::vector<graphics::MaterialData> materials;
    math::BoundingBox localBounds;

    std::string sourcePath;
    float unitScale = 1.0f;
    std::string coordinateConvention = "Right-handed Y-up (meters, bottom-center pivot)";
    std::string author = "RoomCraft 3D";
    std::string license = "MIT";
    std::string sourceReference;
    double importTimeMs = 0.0;

    void recomputeBounds()
    {
        localBounds.reset();
        for (const auto& sub : subMeshes) {
            const auto partBox = sub.meshData.boundingBox.transformed(sub.localTransform.getModelMatrix());
            localBounds.expand(partBox);
        }
    }

    std::uint32_t getTotalVertexCount() const
    {
        std::uint32_t total = 0;
        for (const auto& sub : subMeshes) {
            total += sub.meshData.getVertexCount();
        }
        return total;
    }

    std::uint32_t getTotalIndexCount() const
    {
        std::uint32_t total = 0;
        for (const auto& sub : subMeshes) {
            total += sub.meshData.getIndexCount();
        }
        return total;
    }

    std::uint32_t getTotalTriangleCount() const
    {
        std::uint32_t total = 0;
        for (const auto& sub : subMeshes) {
            total += sub.meshData.getTriangleCount();
        }
        return total;
    }

    std::size_t getUniqueTextureCount() const
    {
        std::vector<std::string> seen;
        for (const auto& mat : materials) {
            if (mat.hasDiffuseTexture && !mat.diffuseTexturePath.empty()) {
                bool exists = false;
                for (const auto& s : seen) {
                    if (s == mat.diffuseTexturePath) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    seen.push_back(mat.diffuseTexturePath);
                }
            }
        }
        return seen.size();
    }

    bool validate(std::string* outError = nullptr) const
    {
        if (subMeshes.empty()) {
            if (outError != nullptr) *outError = "AssetData has no submeshes.";
            return false;
        }
        if (materials.empty()) {
            if (outError != nullptr) *outError = "AssetData has no materials.";
            return false;
        }
        for (const auto& sub : subMeshes) {
            if (sub.materialIndex >= materials.size()) {
                if (outError != nullptr) *outError = "SubMesh materialIndex out of range.";
                return false;
            }
            if (!sub.meshData.validate(outError, false)) {
                return false;
            }
        }
        if (!localBounds.isValid()) {
            if (outError != nullptr) *outError = "AssetData bounding box is invalid.";
            return false;
        }
        return true;
    }

    // Adapter chuyển đổi trực tiếp FurnitureModel từ M2 sang AssetData thống nhất mà không phá vỡ M2
    static AssetData fromFurnitureModel(const std::string& id,
                                        AssetCategory cat,
                                        const geometry::FurnitureModel& model,
                                        double generationTimeMs = 0.0)
    {
        AssetData asset;
        asset.assetId = id;
        asset.displayName = model.name;
        asset.sourceType = AssetSourceType::Procedural;
        asset.category = cat;
        asset.sourcePath = "procedural://" + id;
        asset.unitScale = 1.0f;
        asset.author = "RoomCraft 3D Procedural Geometry Engine (M2)";
        asset.license = "MIT";
        asset.sourceReference = "src/geometry/FurnitureGenerator.cpp";
        asset.importTimeMs = generationTimeMs;

        asset.subMeshes.reserve(model.parts.size());
        asset.materials.reserve(model.parts.size());

        for (std::size_t i = 0; i < model.parts.size(); ++i) {
            const auto& part = model.parts[i];

            graphics::MaterialData mat;
            mat.name = part.name + "_Mat";
            mat.baseColor = part.baseColor;
            mat.specularColor = glm::vec3(0.12f);
            mat.shininess = 26.0f;
            if (part.name == "SeatCushion") {
                mat.specularColor = glm::vec3(0.025f);
                mat.shininess = 8.0f;
            } else if (part.name == "VaseBody_Revolution") {
                mat.specularColor = glm::vec3(0.48f);
                mat.shininess = 72.0f;
            } else if (part.name.rfind("Lamp", 0) == 0) {
                mat.specularColor = glm::vec3(part.name == "LampShade" ? 0.10f : 0.56f);
                mat.shininess = part.name == "LampShade" ? 24.0f : 90.0f;
            }
            asset.materials.push_back(std::move(mat));

            SubMeshData sub;
            sub.name = part.name;
            sub.meshData = part.meshData;
            sub.materialIndex = i;
            sub.localTransform = part.localTransform;
            asset.subMeshes.push_back(std::move(sub));
        }

        asset.recomputeBounds();
        return asset;
    }
};

} // namespace roomcraft::assets
