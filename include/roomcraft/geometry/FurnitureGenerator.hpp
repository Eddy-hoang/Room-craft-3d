#pragma once

#include "roomcraft/geometry/MeshData.hpp"
#include "roomcraft/math/BoundingBox.hpp"
#include "roomcraft/scene/Transform.hpp"

#include <glm/vec4.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace roomcraft::geometry {

// 6 loại nội thất tự sinh bằng thuật toán C++ trong Milestone M2 (5 bắt buộc + 1 nâng cao Bézier Surface)
enum class FurnitureType {
    Desk,
    Chair,
    Bookshelf,
    DeskLamp,
    Vase,
    CurvedChair
};

// Một thành phần con trong mô hình phân cấp nội thất (Hierarchical Modeling):
//   M_world_part = M_parent * localTransform.getModelMatrix()
struct FurniturePart {
    std::string name;
    MeshData meshData;
    scene::Transform localTransform;
    glm::vec4 baseColor{0.75f, 0.75f, 0.75f, 1.0f};
};

// Mô hình nội thất hoàn chỉnh gồm nhiều thành phần con và BoundingBox tổng hợp.
// Quy ước: gốc tọa độ (pivot) của mọi FurnitureModel nằm tại TÂM ĐÁY (0, 0, 0) trên mặt sàn y = 0.
struct FurnitureModel {
    std::string name;
    FurnitureType type = FurnitureType::Desk;
    std::vector<FurniturePart> parts;
    math::BoundingBox boundingBox;

    void recomputeBoundingBox();
    std::uint32_t getTotalVertexCount() const;
    std::uint32_t getTotalIndexCount() const;
    std::uint32_t getTotalTriangleCount() const;
    bool validate(std::string* outError = nullptr) const;
};

// Tham số cho từng loại nội thất (tất cả kích thước tính theo mét)
struct DeskParams {
    float width = 1.40f;
    float depth = 0.70f;
    float height = 0.75f;
    float topThickness = 0.045f;
    float legThickness = 0.055f;
};

struct ChairParams {
    float width = 0.46f;
    float depth = 0.46f;
    float seatHeight = 0.45f;
    float backrestHeight = 0.44f;
    float legRadius = 0.022f;
    int radialSegments = 16;
};

struct BookshelfParams {
    float width = 0.90f;
    float height = 1.80f;
    float depth = 0.32f;
    int shelfCount = 5;
    float panelThickness = 0.026f;
};

struct DeskLampParams {
    float baseRadius = 0.10f;
    float baseHeight = 0.028f;
    float stemHeight = 0.36f;
    float stemRadius = 0.013f;
    float shadeBottomRadius = 0.135f;
    float shadeTopRadius = 0.060f;
    float shadeHeight = 0.16f;
    int radialSegments = 24;
};

struct VaseParams {
    float height = 0.40f;
    float baseRadius = 0.070f;
    float bellyRadius = 0.145f;
    float neckRadius = 0.055f;
    float rimRadius = 0.088f;
    int radialSegments = 32;
    int profileSegments = 20;
};

struct CurvedChairParams {
    float width = 0.50f;
    float depth = 0.48f;
    float seatHeight = 0.45f;
    float backrestHeight = 0.46f;
    float backrestCurvature = 0.11f;
    int uSegments = 12;
    int vSegments = 10;
    int legRadialSegments = 16;
};

// Bộ sinh nội thất phân cấp thuần CPU (không phụ thuộc OpenGL)
class FurnitureGenerator {
public:
    static bool createDesk(const DeskParams& params, FurnitureModel& outModel, std::string* outError = nullptr);
    static bool createChair(const ChairParams& params, FurnitureModel& outModel, std::string* outError = nullptr);
    static bool createBookshelf(const BookshelfParams& params, FurnitureModel& outModel, std::string* outError = nullptr);
    static bool createDeskLamp(const DeskLampParams& params, FurnitureModel& outModel, std::string* outError = nullptr);
    static bool createVase(const VaseParams& params, FurnitureModel& outModel, std::string* outError = nullptr);
    static bool createCurvedChair(const CurvedChairParams& params, FurnitureModel& outModel, std::string* outError = nullptr);
};

} // namespace roomcraft::geometry
