#pragma once

#include "roomcraft/geometry/MeshData.hpp"

#include <string>

namespace roomcraft::geometry {

// Bộ sinh hình học nguyên thủy bằng thuật toán C++ thuần CPU (KHÔNG dùng glutSolid*).
//
// Quy ước hệ tọa độ & pivot thống nhất cho toàn bộ PrimitiveGenerator:
// - Hệ tọa độ tay phải, trục +Y hướng lên, đơn vị mét.
// - Pivot đặt tại TÂM ĐÁY (0, 0, 0):
//     * Plane nằm trên mặt phẳng XZ tại y = 0, pháp tuyến +Y.
//     * Cuboid/Cube, Cylinder, Sphere, Cone, Frustum đều có đáy tiếp xúc y = 0 (y thuộc [0, height]).
// - Toàn bộ tam giác tuân theo chiều ngược chiều kim đồng hồ (CCW) nhìn từ phía ngoài bề mặt.
class PrimitiveGenerator {
public:
    // Sinh mặt phẳng nằm trên XZ (width theo X, depth theo Z, normal +Y)
    static bool createPlane(float width,
                            float depth,
                            int subdivisionsX,
                            int subdivisionsZ,
                            MeshData& outMesh,
                            std::string* outError = nullptr);

    // Sinh hình lập phương cạnh size (tâm đáy tại gốc tọa độ)
    static bool createCube(float size,
                           MeshData& outMesh,
                           std::string* outError = nullptr);

    // Sinh hình hộp chữ nhật 24 đỉnh, 36 chỉ mục với pháp tuyến phẳng & UV riêng từng mặt
    static bool createCuboid(float width,
                             float height,
                             float depth,
                             MeshData& outMesh,
                             std::string* outError = nullptr);

    // Sinh hình trụ quanh trục Y với pháp tuyến mặt bên tỏa tròn và pháp tuyến phẳng riêng cho hai nắp
    static bool createCylinder(float radius,
                               float height,
                               int radialSegments,
                               bool capped,
                               MeshData& outMesh,
                               std::string* outError = nullptr);

    // Sinh hình cầu bằng tọa độ cầu, pháp tuyến giải tích và xử lý hai cực không tạo tam giác suy biến
    static bool createSphere(float radius,
                             int latitudeSegments,
                             int longitudeSegments,
                             MeshData& outMesh,
                             std::string* outError = nullptr);

    // Sinh hình nón (trường hợp đặc biệt của Frustum với topRadius = 0)
    static bool createCone(float radius,
                           float height,
                           int radialSegments,
                           bool capped,
                           MeshData& outMesh,
                           std::string* outError = nullptr);

    // Sinh hình nón cụt (Frustum) với bán kính đáy, bán kính đỉnh, chiều cao và tùy chọn nắp
    static bool createFrustum(float bottomRadius,
                              float topRadius,
                              float height,
                              int radialSegments,
                              bool capBottom,
                              bool capTop,
                              MeshData& outMesh,
                              std::string* outError = nullptr);
};

} // namespace roomcraft::geometry
