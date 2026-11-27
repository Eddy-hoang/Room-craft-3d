#include "roomcraft/geometry/BezierCurve.hpp"
#include "roomcraft/geometry/GeometryUtils.hpp"

#include <cmath>

namespace roomcraft::geometry {

namespace {

bool failWith(std::string* outError, const std::string& message)
{
    if (outError != nullptr) {
        *outError = message;
    }
    return false;
}

bool validateParameterT(float t, std::string* outError)
{
    if (!std::isfinite(t) || t < 0.0f || t > 1.0f) {
        return failWith(outError, "BezierCurve parameter t must be finite and within [0, 1].");
    }
    return true;
}

} // namespace

BezierCurve::BezierCurve()
    : m_controlPoints({
          glm::vec3(-1.0f, 0.2f, 0.0f),
          glm::vec3(-0.4f, 1.4f, 0.0f),
          glm::vec3( 0.4f, 1.4f, 0.0f),
          glm::vec3( 1.0f, 0.2f, 0.0f)
      })
{
}

BezierCurve::BezierCurve(const std::array<glm::vec3, 4>& controlPoints)
    : m_controlPoints(controlPoints)
{
}

BezierCurve::BezierCurve(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3)
    : m_controlPoints({p0, p1, p2, p3})
{
}

void BezierCurve::setControlPoint(int index, const glm::vec3& point)
{
    if (index >= 0 && index < 4) {
        m_controlPoints[static_cast<std::size_t>(index)] = point;
    }
}

bool BezierCurve::isValid() const
{
    for (const auto& p : m_controlPoints) {
        if (!GeometryUtils::isFiniteVec3(p)) {
            return false;
        }
    }
    return true;
}

bool BezierCurve::evaluate(float t, glm::vec3& outPoint, std::string* outError) const
{
    if (!isValid()) {
        return failWith(outError, "BezierCurve contains non-finite control points.");
    }
    if (!validateParameterT(t, outError)) {
        return false;
    }

    const float u = 1.0f - t;
    const float b0 = u * u * u;
    const float b1 = 3.0f * u * u * t;
    const float b2 = 3.0f * u * t * t;
    const float b3 = t * t * t;

    outPoint = b0 * m_controlPoints[0] +
               b1 * m_controlPoints[1] +
               b2 * m_controlPoints[2] +
               b3 * m_controlPoints[3];
    return true;
}

bool BezierCurve::evaluateDeCasteljau(float t, glm::vec3& outPoint, std::string* outError) const
{
    if (!isValid()) {
        return failWith(outError, "BezierCurve contains non-finite control points.");
    }
    if (!validateParameterT(t, outError)) {
        return false;
    }

    // Thuật toán de Casteljau: nội suy tuyến tính lặp qua 3 cấp để thu được điểm trên đường cong
    const float u = 1.0f - t;
    const glm::vec3 q0 = u * m_controlPoints[0] + t * m_controlPoints[1];
    const glm::vec3 q1 = u * m_controlPoints[1] + t * m_controlPoints[2];
    const glm::vec3 q2 = u * m_controlPoints[2] + t * m_controlPoints[3];

    const glm::vec3 r0 = u * q0 + t * q1;
    const glm::vec3 r1 = u * q1 + t * q2;

    outPoint = u * r0 + t * r1;
    return true;
}

bool BezierCurve::derivative(float t, glm::vec3& outDerivative, std::string* outError) const
{
    if (!isValid()) {
        return failWith(outError, "BezierCurve contains non-finite control points.");
    }
    if (!validateParameterT(t, outError)) {
        return false;
    }

    const float u = 1.0f - t;
    outDerivative = 3.0f * u * u * (m_controlPoints[1] - m_controlPoints[0]) +
                    6.0f * u * t * (m_controlPoints[2] - m_controlPoints[1]) +
                    3.0f * t * t * (m_controlPoints[3] - m_controlPoints[2]);
    return true;
}

bool BezierCurve::sample(int segmentCount, std::vector<glm::vec3>& outPoints, std::string* outError) const
{
    if (!isValid()) {
        return failWith(outError, "BezierCurve contains non-finite control points.");
    }
    if (segmentCount < limits::kMinSurfaceSegments || segmentCount > limits::kMaxRadialSegments) {
        return failWith(outError, "BezierCurve segmentCount must be in range [1, 256].");
    }

    outPoints.clear();
    outPoints.reserve(static_cast<std::size_t>(segmentCount + 1));

    for (int i = 0; i <= segmentCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(segmentCount);
        glm::vec3 pt;
        if (!evaluate(t, pt, outError)) {
            return false;
        }
        outPoints.push_back(pt);
    }

    return true;
}

bool BezierCurve::generateLineMesh(int segmentCount, MeshData& outMesh, std::string* outError) const
{
    std::vector<glm::vec3> pts;
    if (!sample(segmentCount, pts, outError)) {
        return false;
    }

    MeshData mesh;
    mesh.topology = graphics::PrimitiveTopology::Lines;
    mesh.vertices.reserve(pts.size());
    mesh.indices.reserve(static_cast<std::size_t>(segmentCount * 2));

    const glm::vec3 up(0.0f, 1.0f, 0.0f);
    for (std::size_t i = 0; i < pts.size(); ++i) {
        const float u = static_cast<float>(i) / static_cast<float>(segmentCount);
        mesh.vertices.push_back({pts[i], up, {u, 0.0f}});
    }

    for (int i = 0; i < segmentCount; ++i) {
        mesh.indices.push_back(static_cast<std::uint32_t>(i));
        mesh.indices.push_back(static_cast<std::uint32_t>(i + 1));
    }

    mesh.recomputeBoundingBox();
    if (!mesh.validate(outError, false)) {
        return false;
    }

    outMesh = std::move(mesh);
    return true;
}

bool BezierCurve::generateControlPolygonMesh(MeshData& outMesh, std::string* outError) const
{
    if (!isValid()) {
        return failWith(outError, "BezierCurve contains non-finite control points.");
    }

    MeshData mesh;
    mesh.topology = graphics::PrimitiveTopology::Lines;
    const glm::vec3 up(0.0f, 1.0f, 0.0f);

    for (int i = 0; i < 4; ++i) {
        mesh.vertices.push_back({m_controlPoints[static_cast<std::size_t>(i)], up, {static_cast<float>(i) / 3.0f, 0.0f}});
    }
    mesh.indices = {0, 1, 1, 2, 2, 3};
    mesh.recomputeBoundingBox();

    if (!mesh.validate(outError, false)) {
        return false;
    }
    outMesh = std::move(mesh);
    return true;
}

} // namespace roomcraft::geometry
