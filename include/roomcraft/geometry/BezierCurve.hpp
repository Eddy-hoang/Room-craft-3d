#pragma once

#include "roomcraft/geometry/MeshData.hpp"

#include <glm/vec3.hpp>

#include <array>
#include <string>
#include <vector>

namespace roomcraft::geometry {

// Đường cong Bézier bậc ba (Cubic Bézier Curve) xác định bởi 4 điểm điều khiển P0, P1, P2, P3:
//   B(t)  = (1-t)^3 P0 + 3(1-t)^2 t P1 + 3(1-t) t^2 P2 + t^3 P3, với t thuộc [0, 1]
//   B'(t) = 3(1-t)^2 (P1 - P0) + 6(1-t)t (P2 - P1) + 3t^2 (P3 - P2)
class BezierCurve {
public:
    BezierCurve();
    explicit BezierCurve(const std::array<glm::vec3, 4>& controlPoints);
    BezierCurve(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3);

    void setControlPoints(const std::array<glm::vec3, 4>& controlPoints) { m_controlPoints = controlPoints; }
    void setControlPoint(int index, const glm::vec3& point);
    const std::array<glm::vec3, 4>& getControlPoints() const { return m_controlPoints; }

    // Kiểm tra 4 điểm điều khiển đều hữu hạn (không NaN/Inf)
    bool isValid() const;

    // Tính vị trí điểm trên đường cong bằng đa thức Bernstein (kiểm tra t thuộc [0, 1])
    bool evaluate(float t, glm::vec3& outPoint, std::string* outError = nullptr) const;

    // Tính vị trí điểm bằng thuật toán de Casteljau để đối chiếu độ chính xác toán học
    bool evaluateDeCasteljau(float t, glm::vec3& outPoint, std::string* outError = nullptr) const;

    // Tính vector đạo hàm bậc nhất B'(t) (tiếp tuyến chưa chuẩn hóa)
    bool derivative(float t, glm::vec3& outDerivative, std::string* outError = nullptr) const;

    // Lấy mẫu (segmentCount + 1) điểm đều theo tham số t trên [0, 1]
    bool sample(int segmentCount, std::vector<glm::vec3>& outPoints, std::string* outError = nullptr) const;

    // Sinh dữ liệu GL_LINES của đường cong để hiển thị trong Geometry Playground
    bool generateLineMesh(int segmentCount, MeshData& outMesh, std::string* outError = nullptr) const;

    // Sinh đa giác điều khiển P0-P1-P2-P3 dạng GL_LINES
    bool generateControlPolygonMesh(MeshData& outMesh, std::string* outError = nullptr) const;

private:
    std::array<glm::vec3, 4> m_controlPoints;
};

} // namespace roomcraft::geometry
