#include "roomcraft/scene/Room.hpp"

#include "roomcraft/geometry/PrimitiveGenerator.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace roomcraft::scene {

namespace {

bool appendCuboid(geometry::MeshData& dst,
                  float w,
                  float h,
                  float d,
                  const glm::vec3& offset,
                  std::string* outError)
{
    geometry::MeshData tmp;
    if (!geometry::PrimitiveGenerator::createCuboid(w, h, d, tmp, outError)) {
        return false;
    }
    const std::uint32_t baseIdx = static_cast<std::uint32_t>(dst.vertices.size());
    dst.vertices.reserve(dst.vertices.size() + tmp.vertices.size());
    for (auto v : tmp.vertices) {
        v.position += offset;
        dst.vertices.push_back(v);
    }
    dst.indices.reserve(dst.indices.size() + tmp.indices.size());
    for (const auto idx : tmp.indices) {
        dst.indices.push_back(baseIdx + idx);
    }
    return true;
}

bool buildArchitecturalDetails(const RoomSettings& settings,
                               geometry::MeshData& outNorthBaseboard,
                               geometry::MeshData& outSouthBaseboard,
                               geometry::MeshData& outEastBaseboard,
                               geometry::MeshData& outWestBaseboard,
                               geometry::MeshData& outWindowFrame,
                               geometry::MeshData& outWindowGlass,
                               std::string* outError)
{
    const float W = settings.width;
    const float L = settings.length;
    const float H = settings.height;

    // 1. Len chân tường (Baseboards) chạy sát mặt trong chân 4 tường: cao 9 cm, dày 1.6 cm
    constexpr float kBbH = 0.090f;
    constexpr float kBbD = 0.016f;
    const float halfW = W * 0.5f;
    const float halfL = L * 0.5f;

    outNorthBaseboard.clear();
    outSouthBaseboard.clear();
    outEastBaseboard.clear();
    outWestBaseboard.clear();

    if (!appendCuboid(outNorthBaseboard, W, kBbH, kBbD, {0.0f, 0.0f, -halfL + kBbD * 0.5f}, outError) ||
        !appendCuboid(outSouthBaseboard, W, kBbH, kBbD, {0.0f, 0.0f,  halfL - kBbD * 0.5f}, outError) ||
        !appendCuboid(outWestBaseboard,  kBbD, kBbH, L, {-halfW + kBbD * 0.5f, 0.0f, 0.0f}, outError) ||
        !appendCuboid(outEastBaseboard,  kBbD, kBbH, L, { halfW - kBbD * 0.5f, 0.0f, 0.0f}, outError)) {
        return false;
    }
    outNorthBaseboard.recomputeBoundingBox();
    outSouthBaseboard.recomputeBoundingBox();
    outWestBaseboard.recomputeBoundingBox();
    outEastBaseboard.recomputeBoundingBox();

    // 2. Cửa sổ kiến trúc trang trí gắn sát mặt trong tường Bắc (-Z), cao trên bàn/đầu giường (>= 1.40 m)
    outWindowFrame.clear();
    outWindowGlass.clear();
    if (H >= 2.55f && W >= 2.80f) {
        const float winW = std::clamp(W * 0.42f, 1.15f, 2.05f);
        const float winH = std::min(1.04f, H - 1.72f);
        const float winY0 = 1.40f;
        if (winH >= 0.65f && winY0 + winH <= H - 0.25f) {
            constexpr float kFrameT = 0.055f;
            constexpr float kFrameD = 0.032f;
            constexpr float kSillD = 0.052f;
            const float zWall = -halfL;
            const float zFrame = zWall + kFrameD * 0.5f;

            // Khung trên, bậu dưới nhô nhẹ, 2 thanh bên và 1 đố dọc trung tâm kiểu Scandinavian
            bool ok = true;
            ok &= appendCuboid(outWindowFrame, winW + 0.08f, kFrameT * 0.95f, kSillD,
                               {0.0f, winY0, zWall + kSillD * 0.5f}, outError);
            ok &= appendCuboid(outWindowFrame, winW, kFrameT, kFrameD,
                               {0.0f, winY0 + winH - kFrameT, zFrame}, outError);
            ok &= appendCuboid(outWindowFrame, kFrameT, winH - kFrameT * 2.0f, kFrameD,
                               {-winW * 0.5f + kFrameT * 0.5f, winY0 + kFrameT, zFrame}, outError);
            ok &= appendCuboid(outWindowFrame, kFrameT, winH - kFrameT * 2.0f, kFrameD,
                               { winW * 0.5f - kFrameT * 0.5f, winY0 + kFrameT, zFrame}, outError);
            // Đố dọc chia 2 cánh + đố ngang mảnh
            ok &= appendCuboid(outWindowFrame, kFrameT * 0.65f, winH - kFrameT * 2.0f, kFrameD * 0.85f,
                               {0.0f, winY0 + kFrameT, zWall + kFrameD * 0.42f}, outError);
            if (!ok) return false;

            const float glassW = winW - kFrameT * 2.0f + 0.01f;
            const float glassH = winH - kFrameT * 2.0f + 0.01f;
            if (!appendCuboid(outWindowGlass, glassW, glassH, 0.010f,
                              {0.0f, winY0 + kFrameT - 0.005f, zWall + 0.012f}, outError)) {
                return false;
            }
            outWindowFrame.recomputeBoundingBox();
            outWindowGlass.recomputeBoundingBox();
        }
    }

    return true;
}

} // namespace

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

    // 3. Trần phòng tùy chọn: đặt phía trên Y = H
    if (!geometry::PrimitiveGenerator::createCuboid(outerW, ft, outerL, outCeiling, outError)) {
        return false;
    }
    translateMeshVertices(outCeiling, glm::vec3(0.0f, H, 0.0f));

    // 4. Lưới tọa độ bên trong sàn tại Y = 0.002m
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

    const int stepsX = static_cast<int>(std::floor(halfW / step));
    for (int i = -stepsX; i <= stepsX; ++i) {
        const float x = static_cast<float>(i) * step;
        pushLine(glm::vec3(x, kGridY, -halfL), glm::vec3(x, kGridY, halfL));
    }
    if (std::abs(static_cast<float>(stepsX) * step - halfW) > 1e-4f) {
        pushLine(glm::vec3(-halfW, kGridY, -halfL), glm::vec3(-halfW, kGridY, halfL));
        pushLine(glm::vec3(halfW, kGridY, -halfL), glm::vec3(halfW, kGridY, halfL));
    }

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

    // Nếu chỉ đổi màu hoặc trạng thái ẩn/hiện tường/trần/cửa sổ -> cập nhật ngay KHÔNG rebuild mesh
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
    geometry::MeshData tempBbN, tempBbS, tempBbE, tempBbW, tempWinFrame, tempWinGlass;

    if (!buildGeometryForSettings(newSettings,
                                  tempFloor,
                                  tempNorth,
                                  tempSouth,
                                  tempEast,
                                  tempWest,
                                  tempCeiling,
                                  tempGrid,
                                  outError) ||
        !buildArchitecturalDetails(newSettings,
                                   tempBbN,
                                   tempBbS,
                                   tempBbE,
                                   tempBbW,
                                   tempWinFrame,
                                   tempWinGlass,
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
    m_northBaseboardMesh = std::move(tempBbN);
    m_southBaseboardMesh = std::move(tempBbS);
    m_eastBaseboardMesh = std::move(tempBbE);
    m_westBaseboardMesh = std::move(tempBbW);
    m_windowFrameMesh = std::move(tempWinFrame);
    m_windowGlassMesh = std::move(tempWinGlass);
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
    geometry::MeshData tempBbN, tempBbS, tempBbE, tempBbW, tempWinFrame, tempWinGlass;

    if (!buildGeometryForSettings(m_settings,
                                  tempFloor,
                                  tempNorth,
                                  tempSouth,
                                  tempEast,
                                  tempWest,
                                  tempCeiling,
                                  tempGrid,
                                  outError) ||
        !buildArchitecturalDetails(m_settings,
                                   tempBbN,
                                   tempBbS,
                                   tempBbE,
                                   tempBbW,
                                   tempWinFrame,
                                   tempWinGlass,
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
    m_northBaseboardMesh = std::move(tempBbN);
    m_southBaseboardMesh = std::move(tempBbS);
    m_eastBaseboardMesh = std::move(tempBbE);
    m_westBaseboardMesh = std::move(tempBbW);
    m_windowFrameMesh = std::move(tempWinFrame);
    m_windowGlassMesh = std::move(tempWinGlass);
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

math::BoundingBox Room::getVisibleBounds() const
{
    math::BoundingBox box;
    for (const auto& pt : collectVisibleCornerPoints()) {
        box.expand(pt);
    }
    return box.isValid() ? box : getExteriorBounds();
}

std::vector<glm::vec3> Room::collectVisibleCornerPoints() const
{
    std::vector<glm::vec3> pts;
    pts.reserve(48);

    auto appendBoxCorners = [&pts](const math::BoundingBox& b) {
        if (!b.isValid()) return;
        pts.emplace_back(b.min.x, b.min.y, b.min.z);
        pts.emplace_back(b.max.x, b.min.y, b.min.z);
        pts.emplace_back(b.min.x, b.max.y, b.min.z);
        pts.emplace_back(b.max.x, b.max.y, b.min.z);
        pts.emplace_back(b.min.x, b.min.y, b.max.z);
        pts.emplace_back(b.max.x, b.min.y, b.max.z);
        pts.emplace_back(b.min.x, b.max.y, b.max.z);
        pts.emplace_back(b.max.x, b.max.y, b.max.z);
    };

    appendBoxCorners(m_floorMesh.boundingBox);
    if (m_settings.isNorthWallVisible()) appendBoxCorners(m_northWallMesh.boundingBox);
    if (m_settings.isSouthWallVisible()) appendBoxCorners(m_southWallMesh.boundingBox);
    if (m_settings.isWestWallVisible())  appendBoxCorners(m_westWallMesh.boundingBox);
    if (m_settings.isEastWallVisible())  appendBoxCorners(m_eastWallMesh.boundingBox);
    if (m_settings.isCeilingVisible())   appendBoxCorners(m_ceilingMesh.boundingBox);

    return pts;
}

} // namespace roomcraft::scene
