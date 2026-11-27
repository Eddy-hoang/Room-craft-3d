#pragma once

#include "roomcraft/assets/FurnitureCatalog.hpp"
#include "roomcraft/scene/ObjectId.hpp"
#include "roomcraft/scene/Scene.hpp"
#include "roomcraft/systems/SelectionState.hpp"

#include <glm/vec3.hpp>

#include <string>

namespace roomcraft::systems {

// Hệ thống đặt đồ nội thất vào phòng (PlacementSystem — thuần CPU).
// Tính điểm đặt mặc định hợp lý trong phạm vi phòng và hỗ trợ Grid Snapping cơ bản (M4-T19).
// Lưu ý: KHÔNG tuyên bố thuật toán tránh va chạm đầy đủ (CollisionSystem thuộc Milestone M8).
class PlacementSystem {
public:
    // Làm tròn tọa độ theo bước lưới (mặc định chỉ snap X và Z, giữ nguyên Y để đặt vật lên mặt bàn/kệ)
    static glm::vec3 snapToGrid(const glm::vec3& position, float gridSpacing, bool snapY = false);

    // Tính vị trí spawn sơ bộ bên trong phạm vi phòng (đối tượng đầu ở tâm, đối tượng sau lệch đều)
    static glm::vec3 computeSpawnPosition(const scene::Scene& scene,
                                          const assets::CatalogEntry& entry,
                                          bool enableGridSnap = false);

    // Tạo và thêm một món nội thất vào Scene, đồng thời cập nhật SelectionState nếu được truyền vào
    static scene::ObjectId placeFurniture(scene::Scene& scene,
                                          const std::string& assetId,
                                          const assets::FurnitureCatalog& catalog,
                                          SelectionState* selection = nullptr,
                                          bool enableGridSnap = false,
                                          std::string* outError = nullptr);
};

} // namespace roomcraft::systems
