#pragma once

#include "roomcraft/geometry/MeshData.hpp"
#include "roomcraft/math/BoundingBox.hpp"
#include "roomcraft/scene/RoomSettings.hpp"

#include <cstdint>
#include <string>

namespace roomcraft::scene {

// Lớp Room quản lý dữ liệu hình học thuần CPU của căn phòng (sàn, 4 tường, trần tùy chọn, lưới sàn)
// được sinh trực tiếp từ PrimitiveGenerator (M2). Không phụ thuộc OpenGL.
class Room {
public:
    Room();

    // Áp dụng cấu hình phòng theo mẫu transactional:
    // - Validate trước khi thay đổi.
    // - Nếu chỉ đổi màu hoặc visibility: cập nhật ngay KHÔNG rebuild mesh.
    // - Nếu đổi kích thước/độ dày/grid: sinh toàn bộ mesh tạm trước, chỉ hoán đổi khi thành công.
    bool applySettings(const RoomSettings& newSettings, std::string* outError = nullptr);

    // Tái sinh hình học theo cấu hình hiện tại
    bool rebuildGeometry(std::string* outError = nullptr);

    void resetToDefault();

    const RoomSettings& getSettings() const { return m_settings; }

    // Phạm vi sử dụng bên trong phòng: X in [-W/2, W/2], Y in [0, H], Z in [-L/2, L/2]
    math::BoundingBox getInteriorBounds() const;

    // Phạm vi tổng thể bao gồm cả độ dày tường và sàn (dùng cho Camera Fit Room)
    math::BoundingBox getExteriorBounds() const;

    const geometry::MeshData& getFloorMesh() const { return m_floorMesh; }
    const geometry::MeshData& getNorthWallMesh() const { return m_northWallMesh; }
    const geometry::MeshData& getSouthWallMesh() const { return m_southWallMesh; }
    const geometry::MeshData& getEastWallMesh() const { return m_eastWallMesh; }
    const geometry::MeshData& getWestWallMesh() const { return m_westWallMesh; }
    const geometry::MeshData& getCeilingMesh() const { return m_ceilingMesh; }
    const geometry::MeshData& getGridMesh() const { return m_gridMesh; }

    // Số phiên bản hình học tăng mỗi khi mesh CPU thay đổi để SceneRenderer đồng bộ lên GPU đúng lúc
    std::uint64_t getGeometryRevision() const { return m_geometryRevision; }

private:
    static bool buildGeometryForSettings(const RoomSettings& settings,
                                         geometry::MeshData& outFloor,
                                         geometry::MeshData& outNorth,
                                         geometry::MeshData& outSouth,
                                         geometry::MeshData& outEast,
                                         geometry::MeshData& outWest,
                                         geometry::MeshData& outCeiling,
                                         geometry::MeshData& outGrid,
                                         std::string* outError);

    static void translateMeshVertices(geometry::MeshData& mesh, const glm::vec3& offset);

private:
    RoomSettings m_settings;
    geometry::MeshData m_floorMesh;
    geometry::MeshData m_northWallMesh;
    geometry::MeshData m_southWallMesh;
    geometry::MeshData m_eastWallMesh;
    geometry::MeshData m_westWallMesh;
    geometry::MeshData m_ceilingMesh;
    geometry::MeshData m_gridMesh;
    std::uint64_t m_geometryRevision = 0;
};

} // namespace roomcraft::scene
