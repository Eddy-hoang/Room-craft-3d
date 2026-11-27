#include "roomcraft/assets/AssetData.hpp"
#include "roomcraft/assets/FurnitureCatalog.hpp"
#include "roomcraft/assets/ModelLoader.hpp"
#include "roomcraft/geometry/FurnitureGenerator.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>

int runAssetCatalogTests()
{
    int failures = 0;
    const auto check = [&failures](const char* name, bool condition) {
        std::cout << "  [Catalog & Hybrid] " << name << ": " << (condition ? "PASS" : "FAIL") << std::endl;
        if (!condition) {
            ++failures;
        }
    };

    roomcraft::assets::FurnitureCatalog catalog;

    // 1. Kiểm tra danh mục có đủ 6 procedural + 8 Blender assets và bộ lọc hoạt động chính xác
    {
        const auto all = catalog.getAllEntries();
        const auto procOnly = catalog.filter(roomcraft::assets::AssetSourceType::Procedural, std::nullopt, "");
        const auto blenderOnly = catalog.filter(roomcraft::assets::AssetSourceType::Imported, std::nullopt, "");
        const auto searchSofa = catalog.filter(std::nullopt, std::nullopt, "sofa");

        check("Catalog registers 6 Procedural + 8 Blender assets (14 total) and filters correctly",
              all.size() == 14 && procOnly.size() == 6 && blenderOnly.size() == 8 && searchSofa.size() == 1 &&
              searchSofa[0]->assetId == "blender-modern-sofa");
    }

    // 2. TC-M3-22: Procedural asset adapter chuyển đổi toàn bộ 6 mẫu M2 sang Unified AssetData
    {
        bool allConverted = true;
        std::string err;
        const auto procEntries = catalog.filter(roomcraft::assets::AssetSourceType::Procedural, std::nullopt, "");
        for (const auto* entry : procEntries) {
            roomcraft::geometry::FurnitureModel model;
            bool genOk = false;
            switch (*entry->proceduralType) {
                case roomcraft::geometry::FurnitureType::Desk:        genOk = roomcraft::geometry::FurnitureGenerator::createDesk({}, model, &err); break;
                case roomcraft::geometry::FurnitureType::Chair:       genOk = roomcraft::geometry::FurnitureGenerator::createChair({}, model, &err); break;
                case roomcraft::geometry::FurnitureType::Bookshelf:   genOk = roomcraft::geometry::FurnitureGenerator::createBookshelf({}, model, &err); break;
                case roomcraft::geometry::FurnitureType::DeskLamp:    genOk = roomcraft::geometry::FurnitureGenerator::createDeskLamp({}, model, &err); break;
                case roomcraft::geometry::FurnitureType::Vase:        genOk = roomcraft::geometry::FurnitureGenerator::createVase({}, model, &err); break;
                case roomcraft::geometry::FurnitureType::CurvedChair: genOk = roomcraft::geometry::FurnitureGenerator::createCurvedChair({}, model, &err); break;
            }
            if (!genOk) {
                allConverted = false;
                continue;
            }
            const auto asset = roomcraft::assets::AssetData::fromFurnitureModel(entry->assetId, entry->category, model);
            allConverted &= asset.validate(&err) &&
                            asset.sourceType == roomcraft::assets::AssetSourceType::Procedural &&
                            std::fabs(asset.localBounds.min.y) < 1e-3f;
        }
        check("TC-M3-22 Procedural adapter converts all 6 M2 models into valid AssetData", allConverted);
    }

    // 3. TC-M3-23: Nạp thực tế toàn bộ 8 Blender OBJ/MTL assets qua Assimp ModelLoader
    {
        const auto blenderEntries = catalog.filter(roomcraft::assets::AssetSourceType::Imported, std::nullopt, "");
        bool allBlenderLoaded = (blenderEntries.size() == 8);
        const std::filesystem::path rootDir(ROOMCRAFT_SOURCE_DIR);

        for (const auto* entry : blenderEntries) {
            roomcraft::assets::ModelLoadOptions opts;
            opts.assetId = entry->assetId;
            opts.displayName = entry->displayName;
            opts.category = entry->category;
            opts.unitScale = entry->unitScale;
            opts.normalizeBottomCenterPivot = true;

            roomcraft::assets::AssetData loaded;
            std::string err;
            const auto fullPath = rootDir / entry->relativeModelPath;
            const bool ok = roomcraft::assets::ModelLoader::loadFromFile(fullPath, opts, loaded, &err);
            if (!ok) {
                std::cerr << "    [FAIL] Could not load " << entry->assetId << ": " << err << std::endl;
                allBlenderLoaded = false;
                continue;
            }

            const glm::vec3 sz = loaded.localBounds.getSize();
            const bool pivotOnFloor = (std::fabs(loaded.localBounds.min.y) < 1e-3f);
            allBlenderLoaded &= pivotOnFloor && loaded.getTotalTriangleCount() > 0;

            std::cout << "    [BlenderAsset] " << entry->assetId
                      << " | subMeshes=" << loaded.subMeshes.size()
                      << " | mats=" << loaded.materials.size()
                      << " | texs=" << loaded.getUniqueTextureCount()
                      << " | verts=" << loaded.getTotalVertexCount()
                      << " | tris=" << loaded.getTotalTriangleCount()
                      << " | size=(" << sz.x << "x" << sz.y << "x" << sz.z << "m)"
                      << " | importMs=" << loaded.importTimeMs << std::endl;
        }

        check("TC-M3-23 All 8 Blender OBJ/MTL furniture assets imported & normalized (y=0)", allBlenderLoaded);
    }

    return failures;
}
