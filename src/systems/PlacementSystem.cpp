#include "roomcraft/systems/PlacementSystem.hpp"

#include <algorithm>
#include <cmath>

namespace roomcraft::systems {

glm::vec3 PlacementSystem::snapToGrid(const glm::vec3& position, float gridSpacing, bool snapY)
{
    if (!std::isfinite(gridSpacing) || gridSpacing <= 1e-4f) {
        return position;
    }
    glm::vec3 snapped = position;
    snapped.x = std::round(position.x / gridSpacing) * gridSpacing;
    snapped.z = std::round(position.z / gridSpacing) * gridSpacing;
    if (snapY) {
        snapped.y = std::round(position.y / gridSpacing) * gridSpacing;
    }
    return snapped;
}

glm::vec3 PlacementSystem::computeSpawnPosition(const scene::Scene& scene,
                                                const assets::CatalogEntry& entry,
                                                bool enableGridSnap)
{
    const auto& settings = scene.getRoom().getSettings();
    const std::size_t index = scene.getObjectCount();

    // Đối tượng đầu đặt tại tâm phòng (0, 0, 0); các đối tượng tiếp theo lệch theo lưới xoắn sơ bộ
    // để không chồng khít lên nhau tại cùng một điểm (chưa phải thuật toán tránh va chạm của M8).
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    if (index > 0) {
        const float ringStep = std::max(0.6f, settings.gridSpacing);
        const int slot = static_cast<int>(index % 9);
        static constexpr float kOffsets[9][2] = {
            { 0.0f,  0.0f},
            { 1.0f,  0.0f},
            {-1.0f,  0.0f},
            { 0.0f,  1.0f},
            { 0.0f, -1.0f},
            { 1.0f,  1.0f},
            {-1.0f,  1.0f},
            { 1.0f, -1.0f},
            {-1.0f, -1.0f}
        };
        const float ringMultiplier = 1.0f + 0.5f * static_cast<float>(index / 9);
        pos.x = kOffsets[slot][0] * ringStep * ringMultiplier;
        pos.z = kOffsets[slot][1] * ringStep * ringMultiplier;
    }

    // Thảm được nâng nhẹ 3mm để tránh z-fighting với mặt trên sàn y = 0
    if (entry.assetId == "blender-carpet") {
        pos.y = 0.003f;
    }

    // Giới hạn sơ bộ trong phạm vi bên trong căn phòng dựa theo kích thước dự kiến
    const float marginX = std::min(entry.defaultDimensions.x * 0.5f, settings.width * 0.45f);
    const float marginZ = std::min(entry.defaultDimensions.z * 0.5f, settings.length * 0.45f);
    const float maxX = std::max(0.0f, settings.width * 0.5f - marginX);
    const float maxZ = std::max(0.0f, settings.length * 0.5f - marginZ);
    pos.x = std::clamp(pos.x, -maxX, maxX);
    pos.z = std::clamp(pos.z, -maxZ, maxZ);

    if (enableGridSnap) {
        pos = snapToGrid(pos, settings.gridSpacing, false);
        pos.x = std::clamp(pos.x, -maxX, maxX);
        pos.z = std::clamp(pos.z, -maxZ, maxZ);
    }

    return pos;
}

scene::ObjectId PlacementSystem::placeFurniture(scene::Scene& scene,
                                                const std::string& assetId,
                                                const assets::FurnitureCatalog& catalog,
                                                SelectionState* selection,
                                                bool enableGridSnap,
                                                std::string* outError)
{
    const assets::CatalogEntry* entry = catalog.findById(assetId);
    if (entry == nullptr) {
        if (outError != nullptr) *outError = "Asset ID not found in catalog: '" + assetId + "'.";
        return scene::kInvalidObjectId;
    }

    scene::Transform transform;
    transform.position = computeSpawnPosition(scene, *entry, enableGridSnap);

    const scene::ObjectId newId = scene.addObject(assetId, "", transform, catalog, outError);
    if (scene::isValidObjectId(newId) && selection != nullptr) {
        selection->selectObject(newId);
    }
    return newId;
}

} // namespace roomcraft::systems
