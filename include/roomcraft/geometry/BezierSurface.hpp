#pragma once

#include "roomcraft/geometry/MeshData.hpp"

#include <glm/vec3.hpp>

#include <array>
#include <string>

namespace roomcraft::geometry {

// Mặt cong Bézier song lập phương (Bicubic Bézier Surface) từ lưới 4x4 điểm điều khiển P[i][j]:
//   S(u, v) = sum_{i=0..3} sum_{j=0..3} B_i^3(u) * B_j^3(v) * P[i][j], với u, v thuộc [0, 1].
// Pháp tuyến bề mặt được tính từ tích có hướng của hai đạo hàm riêng:
//   N(u, v) = normalize(dS/du x dS/dv), khớp đồng nhất với chiều quấn tam giác CCW (i00, i10, i11).
class BezierSurface {
public:
    using ControlGrid = std::array<std::array<glm::vec3, 4>, 4>;

    BezierSurface();
    explicit BezierSurface(const ControlGrid& controlPoints);

    void setControlPoints(const ControlGrid& controlPoints) { m_controlPoints = controlPoints; }
    void setControlPoint(int i, int j, const glm::vec3& point);
    const ControlGrid& getControlPoints() const { return m_controlPoints; }

    bool isValid() const;

    // Đánh giá tọa độ bề mặt S(u, v) và hai vector đạo hàm riêng theo u, v
    bool evaluate(float u, float v, glm::vec3& outPoint, std::string* outError = nullptr) const;
    bool partialDerivativeU(float u, float v, glm::vec3& outDu, std::string* outError = nullptr) const;
    bool partialDerivativeV(float u, float v, glm::vec3& outDv, std::string* outError = nullptr) const;
    bool evaluateNormal(float u, float v, glm::vec3& outNormal, std::string* outError = nullptr) const;

    // Sinh lưới tam giác có normal chuẩn hóa và UV = (u, v)
    bool generateMesh(int uSegments,
                      int vSegments,
                      MeshData& outMesh,
                      std::string* outError = nullptr) const;

    // Sinh lưới đường thẳng biểu diễn mạng điều khiển 4x4 (Control Net)
    bool generateControlNetMesh(MeshData& outMesh, std::string* outError = nullptr) const;

    // Các mẫu mặt cong dựng sẵn cho Geometry Playground và nội thất (Curved Chair)
    static BezierSurface createFlatPatch(float width, float depth);
    static BezierSurface createCurvedWavePatch(float width, float depth, float heightAmplitude);
    static BezierSurface createCurvedBackrestPatch(float width, float height, float curvatureDepth);

private:
    static std::array<float, 4> bernstein(float t);
    static std::array<float, 4> bernsteinDerivative(float t);

private:
    ControlGrid m_controlPoints;
};

} // namespace roomcraft::geometry
