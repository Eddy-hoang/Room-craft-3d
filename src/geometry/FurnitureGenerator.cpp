#include "roomcraft/geometry/FurnitureGenerator.hpp"

#include "roomcraft/geometry/BezierCurve.hpp"
#include "roomcraft/geometry/BezierSurface.hpp"
#include "roomcraft/geometry/GeometryUtils.hpp"
#include "roomcraft/geometry/PrimitiveGenerator.hpp"
#include "roomcraft/geometry/RevolutionSurface.hpp"

namespace roomcraft::geometry {

namespace {

bool failWith(std::string* outError, const std::string& message)
{
    if (outError != nullptr) {
        *outError = message;
    }
    return false;
}

} // namespace

void FurnitureModel::recomputeBoundingBox()
{
    boundingBox.reset();
    for (const auto& part : parts) {
        const auto partWorldBox = part.meshData.boundingBox.transformed(part.localTransform.getModelMatrix());
        boundingBox.expand(partWorldBox);
    }
}

std::uint32_t FurnitureModel::getTotalVertexCount() const
{
    std::uint32_t total = 0;
    for (const auto& part : parts) {
        total += part.meshData.getVertexCount();
    }
    return total;
}

std::uint32_t FurnitureModel::getTotalIndexCount() const
{
    std::uint32_t total = 0;
    for (const auto& part : parts) {
        total += part.meshData.getIndexCount();
    }
    return total;
}

std::uint32_t FurnitureModel::getTotalTriangleCount() const
{
    std::uint32_t total = 0;
    for (const auto& part : parts) {
        total += part.meshData.getTriangleCount();
    }
    return total;
}

bool FurnitureModel::validate(std::string* outError) const
{
    if (parts.empty()) {
        return failWith(outError, "FurnitureModel has no geometry parts.");
    }
    for (const auto& part : parts) {
        if (!part.meshData.validate(outError)) {
            return false;
        }
    }
    if (!boundingBox.isValid()) {
        return failWith(outError, "FurnitureModel bounding box is invalid.");
    }
    return true;
}

bool FurnitureGenerator::createDesk(const DeskParams& params, FurnitureModel& outModel, std::string* outError)
{
    if (!GeometryUtils::isPositiveFinite(params.width, 0.4f, 5.0f) ||
        !GeometryUtils::isPositiveFinite(params.depth, 0.3f, 3.0f) ||
        !GeometryUtils::isPositiveFinite(params.height, 0.4f, 1.8f) ||
        !GeometryUtils::isPositiveFinite(params.topThickness, 0.01f, 0.25f) ||
        !GeometryUtils::isPositiveFinite(params.legThickness, 0.015f, 0.20f)) {
        return failWith(outError, "Desk parameters are out of valid bounds.");
    }

    const float legHeight = params.height - params.topThickness;
    if (legHeight < 0.15f) {
        return failWith(outError, "Desk height must exceed topThickness by at least 0.15m.");
    }
    if (params.legThickness * 2.5f >= params.width || params.legThickness * 2.5f >= params.depth) {
        return failWith(outError, "Desk legThickness is too large relative to desk width/depth.");
    }

    FurnitureModel model;
    model.name = "Procedural Desk";
    model.type = FurnitureType::Desk;

    const glm::vec4 oakTop{0.73f, 0.56f, 0.39f, 1.0f};      // Natural Oak (#B58D60+)
    const glm::vec4 oakBevel{0.63f, 0.47f, 0.32f, 1.0f};    // Viền vát dưới mặt bàn
    const glm::vec4 drawerOak{0.68f, 0.52f, 0.36f, 1.0f};
    const glm::vec4 frameColor{0.20f, 0.22f, 0.26f, 1.0f};  // Matte Charcoal (#25282E+)
    const glm::vec4 brassHandle{0.78f, 0.64f, 0.36f, 1.0f}; // Brushed Brass

    // 1. Mặt bàn 2 lớp tạo cạnh vát tinh tế (giữ chính xác chiều cao params.height & kích thước width x depth)
    const float upperTopH = params.topThickness * 0.65f;
    const float lowerTopH = params.topThickness - upperTopH;
    MeshData topMesh;
    if (!PrimitiveGenerator::createCuboid(params.width, upperTopH, params.depth, topMesh, outError)) {
        return false;
    }
    model.parts.push_back({"Tabletop", std::move(topMesh), scene::Transform({0.0f, legHeight + lowerTopH, 0.0f}), oakTop});

    MeshData bevelMesh;
    if (!PrimitiveGenerator::createCuboid(std::max(0.1f, params.width - 0.016f),
                                          lowerTopH,
                                          std::max(0.1f, params.depth - 0.016f),
                                          bevelMesh,
                                          outError)) {
        return false;
    }
    model.parts.push_back({"TabletopBevel", std::move(bevelMesh), scene::Transform({0.0f, legHeight, 0.0f}), oakBevel});

    // 2. Bốn chân bàn mảnh kèm nút đế chân (đáy đặt chính xác tại y = 0)
    const float capH = std::min(0.018f, legHeight * 0.05f);
    MeshData legProto;
    if (!PrimitiveGenerator::createCuboid(params.legThickness, legHeight, params.legThickness, legProto, outError)) {
        return false;
    }

    const float insetX = (params.width * 0.5f) - (params.legThickness * 0.5f) - 0.035f;
    const float insetZ = (params.depth * 0.5f) - (params.legThickness * 0.5f) - 0.035f;
    const glm::vec3 legPositions[4] = {
        {-insetX, 0.0f,  insetZ},
        { insetX, 0.0f,  insetZ},
        {-insetX, 0.0f, -insetZ},
        { insetX, 0.0f, -insetZ}
    };
    const char* legNames[4] = {"Leg_FrontLeft", "Leg_FrontRight", "Leg_BackLeft", "Leg_BackRight"};

    for (int i = 0; i < 4; ++i) {
        model.parts.push_back({legNames[i], legProto, scene::Transform(legPositions[i]), frameColor});
    }

    // 3. Khung yếm dưới mặt bàn (Apron) & thanh giằng ngang phía sau
    const float beamWidth = insetX * 2.0f - params.legThickness;
    const float sideDepth = insetZ * 2.0f - params.legThickness;
    const float apronH = std::min(0.055f, legHeight * 0.16f);
    if (beamWidth > 0.08f) {
        MeshData rearBeam;
        if (!PrimitiveGenerator::createCuboid(beamWidth, apronH, params.legThickness * 0.55f, rearBeam, outError)) {
            return false;
        }
        model.parts.push_back({"RearSupportBeam",
                               std::move(rearBeam),
                               scene::Transform({0.0f, legHeight - apronH - 0.004f, -insetZ}),
                               frameColor});
    }
    if (sideDepth > 0.08f) {
        MeshData sideApron;
        if (!PrimitiveGenerator::createCuboid(params.legThickness * 0.55f, apronH, sideDepth, sideApron, outError)) {
            return false;
        }
        model.parts.push_back({"Apron_Left", sideApron, scene::Transform({-insetX, legHeight - apronH - 0.004f, 0.0f}), frameColor});
        model.parts.push_back({"Apron_Right", std::move(sideApron), scene::Transform({ insetX, legHeight - apronH - 0.004f, 0.0f}), frameColor});
    }

    // 4. Ngăn kéo mỏng kiểu Scandinavian tích hợp dưới mặt bàn kèm tay nắm đồng thau (nếu bàn đủ rộng)
    if (beamWidth >= 0.65f && sideDepth >= 0.30f && legHeight >= 0.45f) {
        const float drawerW = std::min(0.46f, beamWidth * 0.42f);
        const float drawerH = std::min(0.095f, legHeight * 0.18f);
        const float drawerD = sideDepth * 0.86f;
        const float drawerX = insetX - params.legThickness * 0.5f - drawerW * 0.5f - 0.015f;
        const float drawerY = legHeight - drawerH - 0.005f;
        const float drawerZ = 0.01f;

        MeshData drawerBody;
        if (!PrimitiveGenerator::createCuboid(drawerW, drawerH, drawerD, drawerBody, outError)) {
            return false;
        }
        model.parts.push_back({"DrawerBody", std::move(drawerBody), scene::Transform({drawerX, drawerY, drawerZ}), drawerOak});

        // Tay nắm đồng thau thanh mảnh phía trước ngăn kéo (+Z)
        MeshData handleMesh;
        if (!PrimitiveGenerator::createCuboid(drawerW * 0.34f, 0.012f, 0.016f, handleMesh, outError)) {
            return false;
        }
        model.parts.push_back({"DrawerHandle_Brass",
                               std::move(handleMesh),
                               scene::Transform({drawerX, drawerY + drawerH * 0.44f, drawerZ + drawerD * 0.5f + 0.008f}),
                               brassHandle});
    }
    (void)capH;

    model.recomputeBoundingBox();
    if (!model.validate(outError)) {
        return false;
    }

    outModel = std::move(model);
    return true;
}

bool FurnitureGenerator::createChair(const ChairParams& params, FurnitureModel& outModel, std::string* outError)
{
    if (!GeometryUtils::isPositiveFinite(params.width, 0.30f, 1.20f) ||
        !GeometryUtils::isPositiveFinite(params.depth, 0.30f, 1.20f) ||
        !GeometryUtils::isPositiveFinite(params.seatHeight, 0.25f, 0.90f) ||
        !GeometryUtils::isPositiveFinite(params.backrestHeight, 0.20f, 1.00f) ||
        !GeometryUtils::isPositiveFinite(params.legRadius, 0.01f, 0.08f)) {
        return failWith(outError, "Chair parameters are out of valid bounds.");
    }
    if (params.legRadius * 5.0f >= params.width || params.legRadius * 5.0f >= params.depth) {
        return failWith(outError, "Chair legRadius is too large relative to seat dimensions.");
    }

    const float seatThickness = 0.045f;
    const float legHeight = params.seatHeight - seatThickness;
    if (legHeight < 0.15f) {
        return failWith(outError, "Chair seatHeight is too small.");
    }

    FurnitureModel model;
    model.name = "Procedural Chair";
    model.type = FurnitureType::Chair;

    const glm::vec4 woodLight{0.72f, 0.56f, 0.38f, 1.0f};
    const glm::vec4 woodDark{0.48f, 0.35f, 0.23f, 1.0f};

    // 1. Mặt ngồi (Cuboid)
    MeshData seatMesh;
    if (!PrimitiveGenerator::createCuboid(params.width, seatThickness, params.depth, seatMesh, outError)) {
        return false;
    }
    model.parts.push_back({"Seat", std::move(seatMesh), scene::Transform({0.0f, legHeight, 0.0f}), woodLight});

    // 2. Bốn chân ghế (Cylinder)
    MeshData legProto;
    if (!PrimitiveGenerator::createCylinder(params.legRadius, legHeight, params.radialSegments, true, legProto, outError)) {
        return false;
    }

    const float insetX = params.width * 0.5f - params.legRadius - 0.025f;
    const float insetZ = params.depth * 0.5f - params.legRadius - 0.025f;
    const glm::vec3 legPos[4] = {
        {-insetX, 0.0f,  insetZ},
        { insetX, 0.0f,  insetZ},
        {-insetX, 0.0f, -insetZ},
        { insetX, 0.0f, -insetZ}
    };
    const char* legNames[4] = {"Leg_FL", "Leg_FR", "Leg_BL", "Leg_BR"};
    for (int i = 0; i < 4; ++i) {
        model.parts.push_back({legNames[i], legProto, scene::Transform(legPos[i]), woodDark});
    }

    // 3. Hai trụ tựa lưng phía sau (Cylinder nối từ mặt ngồi lên trên)
    MeshData postProto;
    if (!PrimitiveGenerator::createCylinder(params.legRadius * 0.85f, params.backrestHeight, params.radialSegments, true, postProto, outError)) {
        return false;
    }
    model.parts.push_back({"BackPost_Left", postProto, scene::Transform({-insetX, params.seatHeight, -insetZ}), woodDark});
    model.parts.push_back({"BackPost_Right", postProto, scene::Transform({ insetX, params.seatHeight, -insetZ}), woodDark});

    // 4. Hai thanh tựa ngang (Cuboid)
    const float slatWidth = params.width - 0.02f;
    const float slatHeight = params.backrestHeight * 0.26f;
    MeshData slatProto;
    if (!PrimitiveGenerator::createCuboid(slatWidth, slatHeight, 0.028f, slatProto, outError)) {
        return false;
    }
    model.parts.push_back({"BackSlat_Lower",
                           slatProto,
                           scene::Transform({0.0f, params.seatHeight + params.backrestHeight * 0.28f, -insetZ}),
                           woodLight});
    model.parts.push_back({"BackSlat_Upper",
                           slatProto,
                           scene::Transform({0.0f, params.seatHeight + params.backrestHeight - slatHeight, -insetZ}),
                           woodLight});

    model.recomputeBoundingBox();
    if (!model.validate(outError)) {
        return false;
    }
    outModel = std::move(model);
    return true;
}

bool FurnitureGenerator::createBookshelf(const BookshelfParams& params, FurnitureModel& outModel, std::string* outError)
{
    if (!GeometryUtils::isPositiveFinite(params.width, 0.35f, 4.0f) ||
        !GeometryUtils::isPositiveFinite(params.height, 0.50f, 3.5f) ||
        !GeometryUtils::isPositiveFinite(params.depth, 0.18f, 1.0f) ||
        !GeometryUtils::isPositiveFinite(params.panelThickness, 0.01f, 0.08f)) {
        return failWith(outError, "Bookshelf parameters are out of valid bounds.");
    }
    if (params.shelfCount < 2 || params.shelfCount > 12) {
        return failWith(outError, "Bookshelf shelfCount must be in range [2, 12].");
    }

    const float innerWidth = params.width - 2.0f * params.panelThickness;
    if (innerWidth < 0.15f) {
        return failWith(outError, "Bookshelf panelThickness is too large for the given width.");
    }
    if (static_cast<float>(params.shelfCount) * params.panelThickness >= params.height * 0.8f) {
        return failWith(outError, "Too many shelves for bookshelf height and panelThickness.");
    }

    FurnitureModel model;
    model.name = "Procedural Bookshelf";
    model.type = FurnitureType::Bookshelf;

    // Bảng màu gỗ sồi ấm Scandinavian (Natural Oak + Soft Walnut hậu tủ)
    const glm::vec4 frameWood{0.64f, 0.49f, 0.34f, 1.0f};
    const glm::vec4 shelfWood{0.73f, 0.57f, 0.40f, 1.0f};
    const glm::vec4 backWood{0.50f, 0.38f, 0.26f, 1.0f};

    // 1. Hai vách đứng trái/phải
    MeshData sidePanel;
    if (!PrimitiveGenerator::createCuboid(params.panelThickness, params.height, params.depth, sidePanel, outError)) {
        return false;
    }
    const float sideX = (params.width - params.panelThickness) * 0.5f;
    model.parts.push_back({"SidePanel_Left", sidePanel, scene::Transform({-sideX, 0.0f, 0.0f}), frameWood});
    model.parts.push_back({"SidePanel_Right", sidePanel, scene::Transform({ sideX, 0.0f, 0.0f}), frameWood});

    // 2. Tấm hậu phía sau (-Z)
    const float backThickness = std::max(0.01f, params.panelThickness * 0.45f);
    MeshData backPanel;
    if (!PrimitiveGenerator::createCuboid(innerWidth, params.height, backThickness, backPanel, outError)) {
        return false;
    }
    model.parts.push_back({"BackPanel",
                           std::move(backPanel),
                           scene::Transform({0.0f, 0.0f, -(params.depth * 0.5f) + (backThickness * 0.5f)}),
                           backWood});

    // 3. Chân đế âm (Plinth) phía dưới tạo dáng đồ mộc thật, nằm hoàn toàn trong hộp bao
    const float plinthH = std::min(0.055f, params.height * 0.05f);
    if (plinthH > 0.015f) {
        MeshData plinthMesh;
        if (!PrimitiveGenerator::createCuboid(innerWidth, plinthH, params.panelThickness * 0.8f, plinthMesh, outError)) {
            return false;
        }
        model.parts.push_back({"PlinthBase",
                               std::move(plinthMesh),
                               scene::Transform({0.0f, 0.0f, params.depth * 0.5f - params.panelThickness * 1.1f}),
                               backWood});
    }

    // 4. Vòng lặp sinh các tầng kệ ngang đều nhau từ chân đế lên đỉnh
    const float shelfDepth = params.depth - backThickness;
    MeshData shelfProto;
    if (!PrimitiveGenerator::createCuboid(innerWidth, params.panelThickness, shelfDepth, shelfProto, outError)) {
        return false;
    }

    const float firstShelfY = plinthH;
    const float maxShelfY = params.height - params.panelThickness;
    std::vector<float> shelfTopYs;
    shelfTopYs.reserve(static_cast<std::size_t>(params.shelfCount));
    for (int s = 0; s < params.shelfCount; ++s) {
        const float ratio = static_cast<float>(s) / static_cast<float>(params.shelfCount - 1);
        const float shelfY = firstShelfY + ratio * (maxShelfY - firstShelfY);
        model.parts.push_back({"Shelf_" + std::to_string(s + 1),
                               shelfProto,
                               scene::Transform({0.0f, shelfY, backThickness * 0.5f}),
                               shelfWood});
        shelfTopYs.push_back(shelfY + params.panelThickness);
    }

    // 5. Sinh sách & hộp trang trí tối giản bằng thuật toán C++ (giữ nguyên hộp bao ngoài 0.90x1.80x0.32m)
    if (params.shelfCount >= 4 && innerWidth >= 0.45f && shelfDepth >= 0.16f) {
        const glm::vec4 palette[5] = {
            {0.31f, 0.40f, 0.49f, 1.0f}, // Slate Blue (#465A6E)
            {0.46f, 0.55f, 0.44f, 1.0f}, // Sage Green (#70866B)
            {0.84f, 0.81f, 0.75f, 1.0f}, // Warm Linen (#DAD5CA)
            {0.55f, 0.50f, 0.45f, 1.0f}, // Soft Taupe (#827A6E)
            {0.68f, 0.45f, 0.36f, 1.0f}  // Muted Terracotta
        };

        const float gapH = (shelfTopYs[1] - params.panelThickness) - shelfTopYs[0];
        if (gapH >= 0.22f) {
            int bookIdx = 1;
            const float bookD = std::min(0.18f, shelfDepth * 0.72f);
            const float bookZ = backThickness * 0.5f - (shelfDepth - bookD) * 0.12f;

            // Tầng kệ 1 (dưới cùng): 1 hộp lưu trữ vải linen + cụm 3 quyển sách đứng
            {
                const float y0 = shelfTopYs[0];
                const float boxW = std::min(0.28f, innerWidth * 0.36f);
                const float boxH = std::min(0.17f, gapH * 0.62f);
                MeshData boxMesh;
                if (PrimitiveGenerator::createCuboid(boxW, boxH, bookD * 1.06f, boxMesh, outError)) {
                    model.parts.push_back({"DecorBox_1",
                                           std::move(boxMesh),
                                           scene::Transform({-innerWidth * 0.5f + boxW * 0.5f + 0.04f, y0, bookZ}),
                                           palette[2]});
                }
            }

            // Các tầng kệ giữa: nhóm sách đa dạng chiều cao & màu sắc hài hòa (chỉ dùng C++ Cuboid)
            for (int row = 1; row < params.shelfCount - 1; ++row) {
                const float y0 = shelfTopYs[static_cast<std::size_t>(row)];
                const bool alignLeft = (row % 2 == 1);
                float cursorX = alignLeft ? (-innerWidth * 0.5f + 0.05f) : (innerWidth * 0.5f - 0.05f);
                const int booksInRow = (row % 2 == 1) ? 5 : 4;
                for (int b = 0; b < booksInRow; ++b) {
                    const float bw = 0.026f + 0.006f * static_cast<float>((b + row) % 3);
                    const float bh = std::min(gapH * 0.80f, 0.175f + 0.024f * static_cast<float>((b * 2 + row) % 4));
                    const float cx = alignLeft ? (cursorX + bw * 0.5f) : (cursorX - bw * 0.5f);
                    MeshData bookMesh;
                    if (PrimitiveGenerator::createCuboid(bw, bh, bookD, bookMesh, outError)) {
                        model.parts.push_back({"Book_" + std::to_string(bookIdx++),
                                               std::move(bookMesh),
                                               scene::Transform({cx, y0, bookZ}),
                                               palette[(b + row * 2) % 5]});
                    }
                    cursorX += alignLeft ? (bw + 0.005f) : -(bw + 0.005f);
                }

                // Thêm 1 quyển sách nằm ngang bên phía còn lại tạo nhịp điệu tự nhiên
                const float flatW = std::min(0.20f, innerWidth * 0.25f);
                const float flatX = alignLeft ? (innerWidth * 0.5f - flatW * 0.5f - 0.06f)
                                              : (-innerWidth * 0.5f + flatW * 0.5f + 0.06f);
                MeshData flatBook;
                if (PrimitiveGenerator::createCuboid(flatW, 0.032f, bookD * 0.92f, flatBook, outError)) {
                    model.parts.push_back({"Book_" + std::to_string(bookIdx++),
                                           std::move(flatBook),
                                           scene::Transform({flatX, y0, bookZ}),
                                           palette[(row + 3) % 5]});
                }
            }
        }
    }

    model.recomputeBoundingBox();
    if (!model.validate(outError)) {
        return false;
    }
    outModel = std::move(model);
    return true;
}

bool FurnitureGenerator::createDeskLamp(const DeskLampParams& params, FurnitureModel& outModel, std::string* outError)
{
    if (!GeometryUtils::isPositiveFinite(params.baseRadius, 0.04f, 0.40f) ||
        !GeometryUtils::isPositiveFinite(params.baseHeight, 0.01f, 0.15f) ||
        !GeometryUtils::isPositiveFinite(params.stemHeight, 0.10f, 1.50f) ||
        !GeometryUtils::isPositiveFinite(params.stemRadius, 0.005f, 0.06f) ||
        !GeometryUtils::isPositiveFinite(params.shadeBottomRadius, 0.04f, 0.45f) ||
        !GeometryUtils::isPositiveFinite(params.shadeTopRadius, 0.01f, 0.35f) ||
        !GeometryUtils::isPositiveFinite(params.shadeHeight, 0.05f, 0.50f)) {
        return failWith(outError, "DeskLamp parameters are out of valid bounds.");
    }
    if (params.stemRadius >= params.baseRadius) {
        return failWith(outError, "DeskLamp stemRadius must be smaller than baseRadius.");
    }

    FurnitureModel model;
    model.name = "Procedural Desk Lamp";
    model.type = FurnitureType::DeskLamp;

    const glm::vec4 metalDark{0.24f, 0.27f, 0.32f, 1.0f};
    const glm::vec4 brassAccent{0.78f, 0.64f, 0.36f, 1.0f};
    const glm::vec4 shadeCream{0.92f, 0.87f, 0.74f, 1.0f};

    // 1. Đế đèn hình trụ (Cylinder đặt tại y = 0)
    MeshData baseMesh;
    if (!PrimitiveGenerator::createCylinder(params.baseRadius, params.baseHeight, params.radialSegments, true, baseMesh, outError)) {
        return false;
    }
    model.parts.push_back({"LampBase", std::move(baseMesh), scene::Transform({0.0f, 0.0f, 0.0f}), metalDark});

    // 2. Thân đèn hình trụ (Cylinder nối trên đế đèn)
    MeshData stemMesh;
    if (!PrimitiveGenerator::createCylinder(params.stemRadius, params.stemHeight, params.radialSegments, true, stemMesh, outError)) {
        return false;
    }
    model.parts.push_back({"LampStem", std::move(stemMesh), scene::Transform({0.0f, params.baseHeight, 0.0f}), brassAccent});

    // 3. Khớp trang trí hình cầu (Sphere) tại đỉnh thân đèn
    const float jointRadius = params.stemRadius * 1.8f;
    MeshData jointMesh;
    if (!PrimitiveGenerator::createSphere(jointRadius, 10, params.radialSegments, jointMesh, outError)) {
        return false;
    }
    const float jointBottomY = params.baseHeight + params.stemHeight - jointRadius;
    model.parts.push_back({"LampJoint", std::move(jointMesh), scene::Transform({0.0f, jointBottomY, 0.0f}), brassAccent});

    // 4. Chụp đèn hình nón cụt (Frustum mở hai đầu để mô phỏng chụp đèn)
    MeshData shadeMesh;
    if (!PrimitiveGenerator::createFrustum(params.shadeBottomRadius,
                                           params.shadeTopRadius,
                                           params.shadeHeight,
                                           params.radialSegments,
                                           false,
                                           false,
                                           shadeMesh,
                                           outError)) {
        return false;
    }
    const float shadeY = params.baseHeight + params.stemHeight - (params.shadeHeight * 0.25f);
    model.parts.push_back({"LampShade", std::move(shadeMesh), scene::Transform({0.0f, shadeY, 0.0f}), shadeCream});

    model.recomputeBoundingBox();
    if (!model.validate(outError)) {
        return false;
    }
    outModel = std::move(model);
    return true;
}

bool FurnitureGenerator::createVase(const VaseParams& params, FurnitureModel& outModel, std::string* outError)
{
    if (!GeometryUtils::isPositiveFinite(params.height, 0.10f, 1.50f) ||
        !GeometryUtils::isPositiveFinite(params.baseRadius, 0.02f, 0.50f) ||
        !GeometryUtils::isPositiveFinite(params.bellyRadius, 0.02f, 0.60f) ||
        !GeometryUtils::isPositiveFinite(params.neckRadius, 0.015f, 0.50f) ||
        !GeometryUtils::isPositiveFinite(params.rimRadius, 0.02f, 0.55f)) {
        return failWith(outError, "Vase parameters are out of valid bounds.");
    }

    // Đường cong biên Bézier bậc 3 (X = bán kính r(t), Y = chiều cao y(t)) tạo dáng bình gốm cổ điển
    const BezierCurve profileCurve(
        glm::vec3(params.baseRadius,  0.0f,                  0.0f),
        glm::vec3(params.bellyRadius, params.height * 0.35f, 0.0f),
        glm::vec3(params.neckRadius,  params.height * 0.78f, 0.0f),
        glm::vec3(params.rimRadius,   params.height,         0.0f)
    );

    MeshData vaseMesh;
    if (!RevolutionSurface::generateFromBezierProfile(profileCurve,
                                                      params.profileSegments,
                                                      params.radialSegments,
                                                      true,  // Đóng nắp đáy phẳng tại y = 0
                                                      false, // Miệng bình mở phía trên
                                                      vaseMesh,
                                                      outError)) {
        return false;
    }

    FurnitureModel model;
    model.name = "Procedural Ceramic Vase";
    model.type = FurnitureType::Vase;
    model.parts.push_back({"VaseBody_Revolution", std::move(vaseMesh), scene::Transform(), {0.42f, 0.65f, 0.78f, 1.0f}});
    model.recomputeBoundingBox();

    if (!model.validate(outError)) {
        return false;
    }
    outModel = std::move(model);
    return true;
}

bool FurnitureGenerator::createCurvedChair(const CurvedChairParams& params, FurnitureModel& outModel, std::string* outError)
{
    if (!GeometryUtils::isPositiveFinite(params.width, 0.35f, 1.20f) ||
        !GeometryUtils::isPositiveFinite(params.depth, 0.35f, 1.20f) ||
        !GeometryUtils::isPositiveFinite(params.seatHeight, 0.25f, 0.90f) ||
        !GeometryUtils::isPositiveFinite(params.backrestHeight, 0.25f, 1.00f) ||
        !GeometryUtils::isPositiveFinite(params.backrestCurvature, 0.01f, 0.35f)) {
        return failWith(outError, "CurvedChair parameters are out of valid bounds.");
    }

    const float seatThickness = 0.05f;
    const float legHeight = params.seatHeight - seatThickness;
    const float legRadius = 0.022f;
    if (legHeight < 0.15f) {
        return failWith(outError, "CurvedChair seatHeight is too small.");
    }

    FurnitureModel model;
    model.name = "Procedural Curved Chair (Bezier Surface)";
    model.type = FurnitureType::CurvedChair;

    const glm::vec4 legColor{0.28f, 0.30f, 0.36f, 1.0f};
    const glm::vec4 cushionColor{0.34f, 0.56f, 0.78f, 1.0f};
    const glm::vec4 backrestColor{0.46f, 0.66f, 0.86f, 1.0f};

    // 1. Bốn chân ghế hình trụ
    MeshData legProto;
    if (!PrimitiveGenerator::createCylinder(legRadius, legHeight, params.legRadialSegments, true, legProto, outError)) {
        return false;
    }

    const float insetX = params.width * 0.5f - legRadius - 0.03f;
    const float insetZ = params.depth * 0.5f - legRadius - 0.03f;
    const glm::vec3 legPos[4] = {
        {-insetX, 0.0f,  insetZ},
        { insetX, 0.0f,  insetZ},
        {-insetX, 0.0f, -insetZ},
        { insetX, 0.0f, -insetZ}
    };
    const char* legNames[4] = {"Leg_FL", "Leg_FR", "Leg_BL", "Leg_BR"};
    for (int i = 0; i < 4; ++i) {
        model.parts.push_back({legNames[i], legProto, scene::Transform(legPos[i]), legColor});
    }

    // 2. Đệm ngồi (Cuboid)
    MeshData seatMesh;
    if (!PrimitiveGenerator::createCuboid(params.width, seatThickness, params.depth, seatMesh, outError)) {
        return false;
    }
    model.parts.push_back({"SeatCushion", std::move(seatMesh), scene::Transform({0.0f, legHeight, 0.0f}), cushionColor});

    // 3. Tựa lưng uốn cong sinh bằng Bicubic Bézier Surface
    const BezierSurface backrestPatch =
        BezierSurface::createCurvedBackrestPatch(params.width, params.backrestHeight, params.backrestCurvature);
    MeshData backrestMesh;
    if (!backrestPatch.generateMesh(params.uSegments, params.vSegments, backrestMesh, outError)) {
        return false;
    }

    model.parts.push_back({"CurvedBackrest_BezierSurface",
                           std::move(backrestMesh),
                           scene::Transform({0.0f, params.seatHeight, -(params.depth * 0.5f) + 0.04f}),
                           backrestColor});

    model.recomputeBoundingBox();
    if (!model.validate(outError)) {
        return false;
    }
    outModel = std::move(model);
    return true;
}

} // namespace roomcraft::geometry
