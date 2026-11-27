#include "roomcraft/assets/AssetManager.hpp"

#include "roomcraft/assets/ModelLoader.hpp"
#include "roomcraft/geometry/FurnitureGenerator.hpp"
#include "roomcraft/geometry/GeometryUtils.hpp"

#include <glm/gtc/matrix_inverse.hpp>

#include <chrono>

namespace roomcraft::assets {

AssetManager::~AssetManager()
{
    clear();
}

void AssetManager::setSearchRoots(const std::filesystem::path& executableDir,
                                  const std::filesystem::path& workingDir)
{
    m_executableDir = executableDir;
    m_workingDir = workingDir;
}

std::filesystem::path AssetManager::resolveRelativePath(const std::filesystem::path& relPath) const
{
    if (relPath.is_absolute()) {
        return relPath;
    }

    std::error_code ec;
    if (!m_executableDir.empty()) {
        const auto candidateExe = m_executableDir / relPath;
        if (std::filesystem::exists(candidateExe, ec)) {
            return candidateExe;
        }
    }

    if (!m_workingDir.empty()) {
        const auto candidateCwd = m_workingDir / relPath;
        if (std::filesystem::exists(candidateCwd, ec)) {
            return candidateCwd;
        }
    }

    return m_executableDir.empty() ? relPath : (m_executableDir / relPath);
}

bool AssetManager::buildCpuAssetFromEntry(const CatalogEntry& entry,
                                          AssetData& outData,
                                          std::string* outError) const
{
    if (entry.sourceType == AssetSourceType::Procedural) {
        if (!entry.proceduralType.has_value()) {
            if (outError != nullptr) *outError = "Catalog entry missing proceduralType: " + entry.assetId;
            return false;
        }

        const auto start = std::chrono::steady_clock::now();
        geometry::FurnitureModel model;
        bool ok = false;
        switch (*entry.proceduralType) {
            case geometry::FurnitureType::Desk:        ok = geometry::FurnitureGenerator::createDesk({}, model, outError); break;
            case geometry::FurnitureType::Chair:       ok = geometry::FurnitureGenerator::createChair({}, model, outError); break;
            case geometry::FurnitureType::Bookshelf:   ok = geometry::FurnitureGenerator::createBookshelf({}, model, outError); break;
            case geometry::FurnitureType::DeskLamp:    ok = geometry::FurnitureGenerator::createDeskLamp({}, model, outError); break;
            case geometry::FurnitureType::Vase:        ok = geometry::FurnitureGenerator::createVase({}, model, outError); break;
            case geometry::FurnitureType::CurvedChair: ok = geometry::FurnitureGenerator::createCurvedChair({}, model, outError); break;
            default:
                if (outError != nullptr) *outError = "Unsupported procedural furniture type.";
                return false;
        }
        if (!ok) {
            return false;
        }

        const auto end = std::chrono::steady_clock::now();
        const double ms = std::chrono::duration<double, std::milli>(end - start).count();

        outData = AssetData::fromFurnitureModel(entry.assetId, entry.category, model, ms);
        outData.displayName = entry.displayName;
        outData.author = entry.author;
        outData.license = entry.license;
        outData.sourceReference = entry.sourceReference;
        return outData.validate(outError);
    }

    // Nhập mô hình Blender OBJ/MTL thông qua Assimp ModelLoader
    const std::filesystem::path resolved = resolveRelativePath(entry.relativeModelPath);
    ModelLoadOptions opts;
    opts.assetId = entry.assetId;
    opts.displayName = entry.displayName;
    opts.category = entry.category;
    opts.unitScale = entry.unitScale;
    opts.normalizeBottomCenterPivot = true;
    opts.author = entry.author;
    opts.license = entry.license;
    opts.sourceReference = entry.sourceReference;

    return ModelLoader::loadFromFile(resolved, opts, outData, outError);
}

std::shared_ptr<GpuAsset> AssetManager::uploadCpuAssetToGpu(AssetData cpuData, std::string* outError)
{
    auto gpuAsset = std::make_shared<GpuAsset>();
    gpuAsset->cpuData = std::move(cpuData);
    gpuAsset->subMeshes.reserve(gpuAsset->cpuData.subMeshes.size());

    geometry::MeshData combinedNormals;
    combinedNormals.topology = graphics::PrimitiveTopology::Lines;

    for (const auto& subCpu : gpuAsset->cpuData.subMeshes) {
        GpuSubMesh subGpu;
        subGpu.name = subCpu.name;
        subGpu.materialIndex = subCpu.materialIndex;
        subGpu.localTransform = subCpu.localTransform;

        if (!subGpu.mesh.create(subCpu.meshData.vertices,
                                subCpu.meshData.indices,
                                subCpu.meshData.topology,
                                outError)) {
            gpuAsset->destroyGpuResources();
            return nullptr;
        }

        // Chuẩn bị overlay Vertex Normals (lấy mẫu thưa nếu mesh nhiều đỉnh để giữ trực quan gọn)
        if (subCpu.meshData.topology == graphics::PrimitiveTopology::Triangles) {
            const glm::mat4 localMat = subCpu.localTransform.getModelMatrix();
            const glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(localMat));
            const std::size_t step = (subCpu.meshData.vertices.size() > 600) ? (subCpu.meshData.vertices.size() / 400) : 1;
            const glm::vec3 up(0.0f, 1.0f, 0.0f);
            for (std::size_t i = 0; i < subCpu.meshData.vertices.size(); i += step) {
                const auto& v = subCpu.meshData.vertices[i];
                const glm::vec3 p = glm::vec3(localMat * glm::vec4(v.position, 1.0f));
                const glm::vec3 n = glm::normalize(normalMat * v.normal);
                const auto baseIdx = static_cast<std::uint32_t>(combinedNormals.vertices.size());
                combinedNormals.vertices.push_back({p, up, {0.0f, 0.0f}});
                combinedNormals.vertices.push_back({p + n * 0.06f, up, {1.0f, 0.0f}});
                combinedNormals.indices.push_back(baseIdx);
                combinedNormals.indices.push_back(baseIdx + 1);
            }
        }

        gpuAsset->subMeshes.push_back(std::move(subGpu));
    }

    // Nạp hoặc lấy từ TextureManager cache cho từng Material có diffuse texture
    gpuAsset->materialDiffuseTextures.resize(gpuAsset->cpuData.materials.size());
    for (std::size_t i = 0; i < gpuAsset->cpuData.materials.size(); ++i) {
        const auto& mat = gpuAsset->cpuData.materials[i];
        if (mat.hasDiffuseTexture && !mat.diffuseTexturePath.empty()) {
            bool usedFallback = false;
            gpuAsset->materialDiffuseTextures[i] =
                m_textureManager.getOrLoadTexture(mat.diffuseTexturePath, &usedFallback);
            if (usedFallback) {
                gpuAsset->usedFallbackTexture = true;
            }
        }
    }

    if (gpuAsset->cpuData.localBounds.isValid()) {
        const auto bboxLines = geometry::GeometryUtils::buildBoundingBoxLines(gpuAsset->cpuData.localBounds);
        if (!bboxLines.vertices.empty()) {
            gpuAsset->boundingBoxLines.create(bboxLines.vertices, bboxLines.indices, bboxLines.topology);
        }
    }

    if (!combinedNormals.vertices.empty()) {
        gpuAsset->vertexNormalLines.create(combinedNormals.vertices, combinedNormals.indices, combinedNormals.topology);
    }

    return gpuAsset;
}

std::shared_ptr<GpuAsset> AssetManager::getOrLoadAsset(const std::string& assetId, std::string* outError)
{
    const auto it = m_assetCache.find(assetId);
    if (it != m_assetCache.end() && it->second) {
        return it->second;
    }

    const CatalogEntry* entry = m_catalog.findById(assetId);
    if (entry == nullptr) {
        if (outError != nullptr) {
            *outError = "Asset ID not found in FurnitureCatalog: " + assetId;
        }
        return nullptr;
    }

    AssetData cpuData;
    if (!buildCpuAssetFromEntry(*entry, cpuData, outError)) {
        return nullptr;
    }

    auto uploaded = uploadCpuAssetToGpu(std::move(cpuData), outError);
    if (!uploaded) {
        return nullptr;
    }

    m_assetCache[assetId] = uploaded;
    return uploaded;
}

bool AssetManager::reloadAsset(const std::string& assetId, std::string* outError)
{
    const CatalogEntry* entry = m_catalog.findById(assetId);
    if (entry == nullptr) {
        if (outError != nullptr) {
            *outError = "Cannot reload unregistered assetId: " + assetId;
        }
        return false;
    }

    // Transactional reload: nạp & upload bản mới hoàn chỉnh trước; chỉ thay thế cache khi thành công
    AssetData newCpuData;
    if (!buildCpuAssetFromEntry(*entry, newCpuData, outError)) {
        return false;
    }

    auto newGpuAsset = uploadCpuAssetToGpu(std::move(newCpuData), outError);
    if (!newGpuAsset) {
        return false;
    }

    m_assetCache[assetId] = std::move(newGpuAsset);
    return true;
}

std::shared_ptr<GpuAsset> AssetManager::loadCustomImportedAsset(const std::string& assetId,
                                                                const std::filesystem::path& filePath,
                                                                float unitScale,
                                                                std::string* outError)
{
    CatalogEntry entry;
    entry.assetId = assetId;
    entry.displayName = assetId;
    entry.sourceType = AssetSourceType::Imported;
    entry.category = AssetCategory::Decor;
    entry.relativeModelPath = filePath.string();
    entry.unitScale = unitScale;
    m_catalog.registerEntry(entry);

    // Nếu đã có trong cache trả về bản cache (kiểm thử cache reuse)
    return getOrLoadAsset(assetId, outError);
}

void AssetManager::clear()
{
    for (auto& [id, asset] : m_assetCache) {
        if (asset) {
            asset->destroyGpuResources();
        }
    }
    m_assetCache.clear();
    m_textureManager.clear();
}

} // namespace roomcraft::assets
