#pragma once

#include "roomcraft/assets/FurnitureCatalog.hpp"
#include "roomcraft/scene/Scene.hpp"

#include <cstddef>
#include <string>

namespace roomcraft::scene {

// Ba căn phòng mẫu (Scene Presets) xây dựng hoàn toàn từ RoomSettings + 14 asset độc lập hiện có
enum class ScenePresetType {
    ModernBedroom = 0,
    GamingWorkspace = 1,
    MinimalistLivingRoom = 2
};

inline const char* scenePresetTypeToString(ScenePresetType type)
{
    switch (type) {
        case ScenePresetType::ModernBedroom:        return "Modern Bedroom (5x6x3m)";
        case ScenePresetType::GamingWorkspace:      return "Gaming Workspace (4x5x3m)";
        case ScenePresetType::MinimalistLivingRoom: return "Minimalist Living Room (6x7x3m)";
        default:                                    return "Unknown Preset";
    }
}

class ScenePresets {
public:
    // Áp dụng một trong 3 căn phòng mẫu vào Scene (xóa object cũ, cấu hình RoomSettings và thêm các SceneObject)
    static bool applyPreset(ScenePresetType preset,
                            Scene& scene,
                            const assets::FurnitureCatalog& catalog,
                            std::string* outError = nullptr);

    // Dựng cảnh benchmark hiệu năng với số lượng vật thể chỉ định (10 / 30 / 100)
    // sử dụng lặp lại các asset đã cache để đo hiệu năng Multi-object Rendering
    static bool buildBenchmarkScene(std::size_t objectCount,
                                    Scene& scene,
                                    const assets::FurnitureCatalog& catalog,
                                    std::string* outError = nullptr);
};

} // namespace roomcraft::scene
