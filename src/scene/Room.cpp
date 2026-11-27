#include "roomcraft/scene/Room.hpp"

#include "roomcraft/geometry/PrimitiveGenerator.hpp"

#include <cmath>
#include <utility>

namespace roomcraft::scene {

Room::Room()
{
    std::string err;
    rebuildGeometry(&err);
}

void Room::translateMeshVertices(geometry::MeshData& mesh, const glm::vec3& offset)
{
    for (auto& v : mesh.vertices) {
        v.position += offset;
    }
    mesh.recomputeBoundingBox();
}

bool Room::buildGeometryForSettings(const RoomSettings& settings,
                                    geometry::MeshData& outFloor,
                                    geometry::MeshData& outNorth,
                                    geometry::MeshData& outSouth,
                                    geometry::MeshData& outEast,
                                    geometry::MeshData& outWest,
                                    geometry::MeshData& outCeiling,
                                    geometry::MeshData& outGrid,
                                    std::string* outError)
{
    if (!settings.validate(outError)) {
        return false;
    }

    const float W = settings.width;
    const float L = settings.length;
    const float H = settings.height;
    const float t = settings.wallThickness;
    const float ft = settings.floorThickness;

    const float outerW = W + 2.0f * t;
    const float outerL = L + 2.0f * t;

    // 1. Mặt sàn: mặt trên nằm chính xác tại Y = 0, độ dày kéo xuống Y = -floorThickness
    if (!geometry::PrimitiveGenerator::createCuboid(outerW, ft, outerL, outFloor, outError)) {
        return false;
    }
    translateMeshVertices(outFloor, glm::vec3(0.0f, -ft, 0.0f));

    // 2. Bốn bức tường dựng ra NGOÀI giới hạn sử dụng bên trong [-W/2, W/2] x [-L/2, L/2]
    // Tường Bắc (-Z) và Tường Nam (+Z) phủ trọn chiều rộng ngoài (W + 2t) để bịt kín 4 góc,
    // Tường Tây (-X) và Tường Đông (+X) nằm khít giữa hai tường Bắc/Nam (chiều dài L) -> không khe hở, không z-fighting.
    if (!geometry::PrimitiveGenerator::createCuboid(outerW, H, t, outNorth, outError)) {
        return false;
    }
    translateMeshVertices(outNorth, glm::vec3(0.0f, 0.0f, -L * 0.5f - t * 0.5f));

    if (!geometry::PrimitiveGenerator::createCuboid(outerW, H, t, outSouth, outError)) {
        return false;
    }
    translateMeshVertices(outSouth, glm::vec3(0.0f, 0.0f, L * 0.5f + t * 0.5f));

    if (!geometry::PrimitiveGenerator::createCuboid(t, H, L, outWest, outError)) {
        return false;
    }
    translateMeshVertices(outWest, glm::vec3(-W * 0.5f - t * 0.5f, 0.0f, 0.0f));

    if (!geometry::PrimitiveGenerator::createCuboid(t, H, L, outEast, outError)) {
        return false;
    }
    translateMeshVertices(outEast, glm::vec3(W * 0.5f + t * 0.5f, 0.0f, 0.0f));

    // 3. Trần phòng tùy chọn: đặt phía trên Y = H để không chồng lên mặt trên của tường
    if (!geometry::PrimitiveGenerator::createCuboid(outerW, ft, outerL, outCeiling, outError)) {
        return false;
    }
    translateMeshVertices(outCeiling, glm::vec3(0.0f, H, 0.0f));

    // 4. Lưới tọa độ bên trong sàn tại Y = 0.002m (lệch nhẹ 2mm tránh z-fighting với mặt trên sàn)
    outGrid.clear();
    outGrid.topology = graphics::PrimitiveTopology::Lines;

    constexpr float kGridY = 0.002f;
    const float halfW = W * 0.5f;
    const float halfL = L * 0.5f;
    const float step = settings.gridSpacing;

    auto pushLine = [&](const glm::vec3& a, const glm::vec3& b) {
        const std::uint32_t base = static_cast<std::uint32_t>(outGrid.vertices.size());
        outGrid.vertices.push_back({a, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f)});
        outGrid.vertices.push_back({b, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 1.0f)});
        outGrid.indices.push_back(base);
        outGrid.indices.push_back(base + 1);
    };

    // Các đường song song trục Z (dọc theo X từ -halfW tới +halfW)
    const int stepsX = static_cast<int>(std::floor(halfW / step));
    for (int i = -stepsX; i <= stepsX; ++i) {
        const float x = static_cast<float>(i) * step;
        pushLine(glm::vec3(x, kGridY, -halfL), glm::vec3(x, kGridY, halfL));
    }
    // Viền biên chính xác của sàn theo X
    if (std::abs(static_cast<float>(stepsX) * step - halfW) > 1e-4f) {
        pushLine(glm::vec3(-halfW, kGridY, -halfL), glm::vec3(-halfW, kGridY, halfL));
        pushLine(glm::vec3(halfW, kGridY, -halfL), glm::vec3(halfW, kGridY, halfL));
    }

    // Các đường song song trục X (dọc theo Z từ -halfL tới +halfL)
    const int stepsZ = static_cast<int>(std::floor(halfL / step));
    for (int j = -stepsZ; j <= stepsZ; ++j) {
        const float z = static_cast<float>(j) * step;
        pushLine(glm::vec3(-halfW, kGridY, z), glm::vec3(halfW, kGridY, z));
    }
    if (std::abs(static_cast<float>(stepsZ) * step - halfL) > 1e-4f) {
        pushLine(glm::vec3(-halfW, kGridY, -halfL), glm::vec3(halfW, kGridY, -halfL));
        pushLine(glm::vec3(-halfW, kGridY, halfL), glm::vec3(halfW, kGridY, halfL));
    }

    outGrid.recomputeBoundingBox();

    return outFloor.validate(outError) &&
           outNorth.validate(outError) &&
           outSouth.validate(outError) &&
           outEast.validate(outError) &&
           outWest.validate(outError) &&
           outCeiling.validate(outError) &&
           outGrid.validate(outError, false);
}

bool Room::applySettings(const RoomSettings& newSettings, std::string* outError)
{
    if (!newSettings.validate(outError)) {
        return false;
    }

    // Nếu chỉ đổi màu hoặc trạng thái ẩn/hiện tường/trần -> cập nhật ngay KHÔNG rebuild mesh
    if (m_geometryRevision > 0 && m_settings.geometryEquals(newSettings)) {
        m_settings = newSettings;
        return true;
    }

    // Transactional rebuild: tạo toàn bộ lưới tạm trước, chỉ hoán đổi khi tất cả hợp lệ
    geometry::MeshData tempFloor;
    geometry::MeshData tempNorth;
    geometry::MeshData tempSouth;
    geometry::MeshData tempEast;
    geometry::MeshData tempWest;
    geometry::MeshData tempCeiling;
    geometry::MeshData tempGrid;

    if (!buildGeometryForSettings(newSettings,
                                  tempFloor,
                                  tempNorth,
                                  tempSouth,
                                  tempEast,
                                  tempWest,
                                  tempCeiling,
                                  tempGrid,
                                  outError)) {
        return false;
    }

    m_settings = newSettings;
    m_floorMesh = std::move(tempFloor);
    m_northWallMesh = std::move(tempNorth);
    m_southWallMesh = std::move(tempSouth);
    m_eastWallMesh = std::move(tempEast);
    m_westWallMesh = std::move(tempWest);
    m_ceilingMesh = std::move(tempCeiling);
    m_gridMesh = std::move(tempGrid);
    ++m_geometryRevision;
    return true;
}

bool Room::rebuildGeometry(std::string* outError)
{
    geometry::MeshData tempFloor;
    geometry::MeshData tempNorth;
    geometry::MeshData tempSouth;
    geometry::MeshData tempEast;
    geometry::MeshData tempWest;
    geometry::MeshData tempCeiling;
    geometry::MeshData tempGrid;

    if (!buildGeometryForSettings(m_settings,
                                  tempFloor,
                                  tempNorth,
                                  tempSouth,
                                  tempEast,
                                  tempWest,
                                  tempCeiling,
                                  tempGrid,
                                  outError)) {
        return false;
    }

    m_floorMesh = std::move(tempFloor);
    m_northWallMesh = std::move(tempNorth);
    m_southWallMesh = std::move(tempSouth);
    m_eastWallMesh = std::move(tempEast);
    m_westWallMesh = std::move(tempWest);
    m_ceilingMesh = std::move(tempCeiling);
    m_gridMesh = std::move(tempGrid);
    ++m_geometryRevision;
    return true;
}

void Room::resetToDefault()
{
    RoomSettings defaults;
    std::string err;
    applySettings(defaults, &err);
}

math::BoundingBox Room::getInteriorBounds() const
{
    math::BoundingBox box;
    box.expand(glm::vec3(-m_settings.width * 0.5f, 0.0f, -m_settings.length * 0.5f));
    box.expand(glm::vec3(m_settings.width * 0.5f, m_settings.height, m_settings.length * 0.5f));
    return box;
}

math::BoundingBox Room::getExteriorBounds() const
{
    const float halfOuterW = m_settings.width * 0.5f + m_settings.wallThickness;
    const float halfOuterL = m_settings.length * 0.5f + m_settings.wallThickness;
    math::BoundingBox box;
    box.expand(glm::vec3(-halfOuterW, -m_settings.floorThickness, -halfOuterL));
    box.expand(glm::vec3(halfOuterW, m_settings.height, halfOuterL));
    return box;
}

} // namespace roomcraft::scene
