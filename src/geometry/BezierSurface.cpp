#include "roomcraft/geometry/BezierSurface.hpp"
#include "roomcraft/geometry/GeometryUtils.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
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

bool validateUV(float u, float v, std::string* outError)
{
    if (!std::isfinite(u) || !std::isfinite(v) || u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f) {
        return failWith(outError, "BezierSurface parameters (u, v) must be finite and in [0, 1].");
    }
    return true;
}

} // namespace

BezierSurface::BezierSurface()
    : m_controlPoints(createCurvedWavePatch(1.6f, 1.6f, 0.45f).m_controlPoints)
{
}

BezierSurface::BezierSurface(const ControlGrid& controlPoints)
    : m_controlPoints(controlPoints)
{
}

void BezierSurface::setControlPoint(int i, int j, const glm::vec3& point)
{
    if (i >= 0 && i < 4 && j >= 0 && j < 4) {
        m_controlPoints[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] = point;
    }
}

bool BezierSurface::isValid() const
{
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (!GeometryUtils::isFiniteVec3(m_controlPoints[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)])) {
                return false;
            }
        }
    }
    return true;
}

std::array<float, 4> BezierSurface::bernstein(float t)
{
    const float s = 1.0f - t;
    return {
        s * s * s,
        3.0f * s * s * t,
        3.0f * s * t * t,
        t * t * t
    };
}

std::array<float, 4> BezierSurface::bernsteinDerivative(float t)
{
    const float s = 1.0f - t;
    return {
        -3.0f * s * s,
         3.0f * s * s - 6.0f * s * t,
         6.0f * s * t - 3.0f * t * t,
         3.0f * t * t
    };
}

bool BezierSurface::evaluate(float u, float v, glm::vec3& outPoint, std::string* outError) const
{
    if (!isValid()) {
        return failWith(outError, "BezierSurface contains non-finite control points.");
    }
    if (!validateUV(u, v, outError)) {
        return false;
    }

    const auto bu = bernstein(u);
    const auto bv = bernstein(v);

    glm::vec3 point(0.0f);
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            point += (bu[static_cast<std::size_t>(i)] * bv[static_cast<std::size_t>(j)]) *
                     m_controlPoints[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)];
        }
    }

    outPoint = point;
    return true;
}

bool BezierSurface::partialDerivativeU(float u, float v, glm::vec3& outDu, std::string* outError) const
{
    if (!isValid()) {
        return failWith(outError, "BezierSurface contains non-finite control points.");
    }
    if (!validateUV(u, v, outError)) {
        return false;
    }

    const auto dbu = bernsteinDerivative(u);
    const auto bv = bernstein(v);

    glm::vec3 du(0.0f);
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            du += (dbu[static_cast<std::size_t>(i)] * bv[static_cast<std::size_t>(j)]) *
                  m_controlPoints[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)];
        }
    }

    outDu = du;
    return true;
}

bool BezierSurface::partialDerivativeV(float u, float v, glm::vec3& outDv, std::string* outError) const
{
    if (!isValid()) {
        return failWith(outError, "BezierSurface contains non-finite control points.");
    }
    if (!validateUV(u, v, outError)) {
        return false;
    }

    const auto bu = bernstein(u);
    const auto dbv = bernsteinDerivative(v);

    glm::vec3 dv(0.0f);
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            dv += (bu[static_cast<std::size_t>(i)] * dbv[static_cast<std::size_t>(j)]) *
                  m_controlPoints[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)];
        }
    }

    outDv = dv;
    return true;
}

bool BezierSurface::evaluateNormal(float u, float v, glm::vec3& outNormal, std::string* outError) const
{
    glm::vec3 du;
    glm::vec3 dv;
    if (!partialDerivativeU(u, v, du, outError) || !partialDerivativeV(u, v, dv, outError)) {
        return false;
    }

    glm::vec3 crossVec = glm::cross(du, dv);
    float len = glm::length(crossVec);

    // Nếu đạo hàm suy biến tại biên/góc (ví dụ 2 điểm điều khiển trùng nhau), dịch nhẹ vào trong
    if (len < 1e-6f) {
        const float epsU = (u < 0.5f) ? std::min(1.0f, u + 1e-3f) : std::max(0.0f, u - 1e-3f);
        const float epsV = (v < 0.5f) ? std::min(1.0f, v + 1e-3f) : std::max(0.0f, v - 1e-3f);
        if (partialDerivativeU(epsU, epsV, du) && partialDerivativeV(epsU, epsV, dv)) {
            crossVec = glm::cross(du, dv);
            len = glm::length(crossVec);
        }
    }

    if (!std::isfinite(len) || len < 1e-6f) {
        return failWith(outError, "Degenerate BezierSurface patch: cannot compute valid surface normal.");
    }

    outNormal = crossVec / len;
    return true;
}

bool BezierSurface::generateMesh(int uSegments,
                                 int vSegments,
                                 MeshData& outMesh,
                                 std::string* outError) const
{
    if (!isValid()) {
        return failWith(outError, "BezierSurface contains non-finite control points.");
    }
    if (uSegments < limits::kMinSurfaceSegments || uSegments > limits::kMaxSurfaceSegments ||
        vSegments < limits::kMinSurfaceSegments || vSegments > limits::kMaxSurfaceSegments) {
        return failWith(outError, "BezierSurface uSegments and vSegments must be in range [1, 128].");
    }

    MeshData mesh;
    mesh.topology = graphics::PrimitiveTopology::Triangles;

    const int cols = uSegments + 1;
    const int rows = vSegments + 1;
    mesh.vertices.reserve(static_cast<std::size_t>(cols * rows));
    mesh.indices.reserve(static_cast<std::size_t>(uSegments * vSegments * 6));

    for (int iv = 0; iv < rows; ++iv) {
        const float v = static_cast<float>(iv) / static_cast<float>(vSegments);
        for (int iu = 0; iu < cols; ++iu) {
            const float u = static_cast<float>(iu) / static_cast<float>(uSegments);

            glm::vec3 pos;
            glm::vec3 normal;
            if (!evaluate(u, v, pos, outError) || !evaluateNormal(u, v, normal, outError)) {
                return false;
            }
            mesh.vertices.push_back({pos, normal, {u, v}});
        }
    }

    for (int iv = 0; iv < vSegments; ++iv) {
        for (int iu = 0; iu < uSegments; ++iu) {
            const auto i00 = static_cast<std::uint32_t>(iv * cols + iu);
            const auto i10 = static_cast<std::uint32_t>(iv * cols + (iu + 1));
            const auto i01 = static_cast<std::uint32_t>((iv + 1) * cols + iu);
            const auto i11 = static_cast<std::uint32_t>((iv + 1) * cols + (iu + 1));

            // (p10 - p00) x (p11 - p00) cùng hướng với (dS/du x dS/dv)
            mesh.indices.push_back(i00);
            mesh.indices.push_back(i10);
            mesh.indices.push_back(i11);

            mesh.indices.push_back(i00);
            mesh.indices.push_back(i11);
            mesh.indices.push_back(i01);
        }
    }

    mesh.recomputeBoundingBox();
    if (!mesh.validate(outError)) {
        return false;
    }

    outMesh = std::move(mesh);
    return true;
}

bool BezierSurface::generateControlNetMesh(MeshData& outMesh, std::string* outError) const
{
    if (!isValid()) {
        return failWith(outError, "BezierSurface contains non-finite control points.");
    }

    MeshData mesh;
    mesh.topology = graphics::PrimitiveTopology::Lines;
    const glm::vec3 up(0.0f, 1.0f, 0.0f);

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            mesh.vertices.push_back({m_controlPoints[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)],
                                     up,
                                     {static_cast<float>(i) / 3.0f, static_cast<float>(j) / 3.0f}});
        }
    }

    // Các đoạn nối dọc theo hướng u và hướng v tạo mạng lưới điều khiển 4x4
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            mesh.indices.push_back(static_cast<std::uint32_t>(i * 4 + j));
            mesh.indices.push_back(static_cast<std::uint32_t>(i * 4 + j + 1));
        }
    }
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j) {
            mesh.indices.push_back(static_cast<std::uint32_t>(i * 4 + j));
            mesh.indices.push_back(static_cast<std::uint32_t>((i + 1) * 4 + j));
        }
    }

    mesh.recomputeBoundingBox();
    if (!mesh.validate(outError, false)) {
        return false;
    }
    outMesh = std::move(mesh);
    return true;
}

BezierSurface BezierSurface::createFlatPatch(float width, float depth)
{
    ControlGrid grid{};
    const float halfW = width * 0.5f;
    const float halfD = depth * 0.5f;

    // u tăng theo +X, v tăng theo -Z => dS/du x dS/dv hướng +Y
    for (int i = 0; i < 4; ++i) {
        const float u = static_cast<float>(i) / 3.0f;
        const float x = -halfW + u * width;
        for (int j = 0; j < 4; ++j) {
            const float v = static_cast<float>(j) / 3.0f;
            const float z = halfD - v * depth;
            grid[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] = glm::vec3(x, 0.0f, z);
        }
    }
    return BezierSurface(grid);
}

BezierSurface BezierSurface::createCurvedWavePatch(float width, float depth, float heightAmplitude)
{
    BezierSurface patch = createFlatPatch(width, depth);
    // Tạo độ cong lồi mượt ở giữa và uốn nhẹ ở các cạnh
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            const bool innerU = (i == 1 || i == 2);
            const bool innerV = (j == 1 || j == 2);
            float y = 0.25f;
            if (innerU && innerV) {
                y += heightAmplitude;
            } else if (innerU || innerV) {
                y += heightAmplitude * 0.35f;
            }
            auto pt = patch.m_controlPoints[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)];
            pt.y = y;
            patch.m_controlPoints[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] = pt;
        }
    }
    return patch;
}

BezierSurface BezierSurface::createCurvedBackrestPatch(float width, float height, float curvatureDepth)
{
    // Tựa lưng ghế cong ôm lưng: u chạy theo chiều rộng (-W/2 -> +W/2), v chạy từ dưới lên trên (0 -> height)
    // dS/du hướng +X, dS/dv hướng +Y => dS/du x dS/dv hướng +Z (hướng về phía người ngồi)
    ControlGrid grid{};
    const float halfW = width * 0.5f;

    for (int i = 0; i < 4; ++i) {
        const float u = static_cast<float>(i) / 3.0f;
        const float x = -halfW + u * width;
        // Hai mép trái/phải ôm ra phía trước (+Z), phần giữa lõm nhẹ về sau (-Z)
        const float horizontalCurve = (i == 1 || i == 2) ? -curvatureDepth : (curvatureDepth * 0.35f);

        for (int j = 0; j < 4; ++j) {
            const float v = static_cast<float>(j) / 3.0f;
            const float y = v * height;
            // Nghiêng nhẹ ra sau khi lên cao để tạo tư thế ngồi tự nhiên
            const float lumbarCurve = -v * (curvatureDepth * 0.65f) + ((j == 1) ? curvatureDepth * 0.22f : 0.0f);
            grid[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] =
                glm::vec3(x, y, horizontalCurve + lumbarCurve);
        }
    }
    return BezierSurface(grid);
}

} // namespace roomcraft::geometry
