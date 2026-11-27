#include "roomcraft/geometry/PrimitiveGenerator.hpp"
#include "roomcraft/geometry/GeometryUtils.hpp"

#include <glm/geometric.hpp>

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

} // namespace

bool PrimitiveGenerator::createPlane(float width,
                                     float depth,
                                     int subdivisionsX,
                                     int subdivisionsZ,
                                     MeshData& outMesh,
                                     std::string* outError)
{
    if (!GeometryUtils::isPositiveFinite(width) || !GeometryUtils::isPositiveFinite(depth)) {
        return failWith(outError, "Plane width and depth must be positive finite values.");
    }
    if (subdivisionsX < limits::kMinSurfaceSegments || subdivisionsX > limits::kMaxSurfaceSegments ||
        subdivisionsZ < limits::kMinSurfaceSegments || subdivisionsZ > limits::kMaxSurfaceSegments) {
        return failWith(outError, "Plane subdivisions out of allowed range [1, 128].");
    }

    MeshData mesh;
    mesh.topology = graphics::PrimitiveTopology::Triangles;

    const int cols = subdivisionsX + 1;
    const int rows = subdivisionsZ + 1;
    mesh.vertices.reserve(static_cast<std::size_t>(cols * rows));
    mesh.indices.reserve(static_cast<std::size_t>(subdivisionsX * subdivisionsZ * 6));

    const float halfW = width * 0.5f;
    const float halfD = depth * 0.5f;
    const glm::vec3 upNormal(0.0f, 1.0f, 0.0f);

    // Sinh lưới đỉnh trên mặt phẳng XZ (y = 0):
    // u đi từ -halfW -> +halfW (+X), v đi từ +halfD -> -halfD (-Z) để tam giác đạt CCW nhìn từ +Y
    for (int iz = 0; iz < rows; ++iz) {
        const float v = static_cast<float>(iz) / static_cast<float>(subdivisionsZ);
        const float z = halfD - v * depth;
        for (int ix = 0; ix < cols; ++ix) {
            const float u = static_cast<float>(ix) / static_cast<float>(subdivisionsX);
            const float x = -halfW + u * width;
            mesh.vertices.push_back({{x, 0.0f, z}, upNormal, {u, v}});
        }
    }

    for (int iz = 0; iz < subdivisionsZ; ++iz) {
        for (int ix = 0; ix < subdivisionsX; ++ix) {
            const auto i00 = static_cast<std::uint32_t>(iz * cols + ix);
            const auto i10 = static_cast<std::uint32_t>(iz * cols + (ix + 1));
            const auto i01 = static_cast<std::uint32_t>((iz + 1) * cols + ix);
            const auto i11 = static_cast<std::uint32_t>((iz + 1) * cols + (ix + 1));

            // (i10 - i00) x (i11 - i00) = (+dx, 0, 0) x (+dx, 0, -dz) = (0, +dx*dz, 0) -> +Y (CCW)
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

bool PrimitiveGenerator::createCube(float size, MeshData& outMesh, std::string* outError)
{
    return createCuboid(size, size, size, outMesh, outError);
}

bool PrimitiveGenerator::createCuboid(float width,
                                      float height,
                                      float depth,
                                      MeshData& outMesh,
                                      std::string* outError)
{
    if (!GeometryUtils::isPositiveFinite(width) ||
        !GeometryUtils::isPositiveFinite(height) ||
        !GeometryUtils::isPositiveFinite(depth)) {
        return failWith(outError, "Cuboid width, height, and depth must be positive finite values.");
    }

    const float hx = width * 0.5f;
    const float hz = depth * 0.5f;
    const float y0 = 0.0f;
    const float y1 = height;

    // 24 đỉnh (4 đỉnh/mặt) để mỗi trong 6 mặt có vector pháp tuyến phẳng và UV độc lập [0, 1]
    MeshData mesh;
    mesh.topology = graphics::PrimitiveTopology::Triangles;
    mesh.vertices = {
        // Mặt trước (+Z)
        {{-hx, y0,  hz}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}},
        {{ hx, y0,  hz}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}},
        {{ hx, y1,  hz}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}},
        {{-hx, y1,  hz}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}},

        // Mặt sau (-Z)
        {{ hx, y0, -hz}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}},
        {{-hx, y0, -hz}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}},
        {{-hx, y1, -hz}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}},
        {{ hx, y1, -hz}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}},

        // Mặt trái (-X)
        {{-hx, y0, -hz}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
        {{-hx, y0,  hz}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
        {{-hx, y1,  hz}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
        {{-hx, y1, -hz}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},

        // Mặt phải (+X)
        {{ hx, y0,  hz}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
        {{ hx, y0, -hz}, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
        {{ hx, y1, -hz}, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
        {{ hx, y1,  hz}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},

        // Mặt trên (+Y)
        {{-hx, y1,  hz}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}},
        {{ hx, y1,  hz}, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}},
        {{ hx, y1, -hz}, { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}},
        {{-hx, y1, -hz}, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}},

        // Mặt dưới (-Y)
        {{-hx, y0, -hz}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}},
        {{ hx, y0, -hz}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}},
        {{ hx, y0,  hz}, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}},
        {{-hx, y0,  hz}, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}}
    };

    mesh.indices = {
        0,  1,  2,  2,  3,  0,
        4,  5,  6,  6,  7,  4,
        8,  9,  10, 10, 11, 8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20
    };

    mesh.recomputeBoundingBox();
    if (!mesh.validate(outError)) {
        return false;
    }
    outMesh = std::move(mesh);
    return true;
}

bool PrimitiveGenerator::createCylinder(float radius,
                                        float height,
                                        int radialSegments,
                                        bool capped,
                                        MeshData& outMesh,
                                        std::string* outError)
{
    if (!GeometryUtils::isPositiveFinite(radius)) {
        return failWith(outError, "Cylinder radius must be positive and finite.");
    }
    return createFrustum(radius, radius, height, radialSegments, capped, capped, outMesh, outError);
}

bool PrimitiveGenerator::createCone(float radius,
                                    float height,
                                    int radialSegments,
                                    bool capped,
                                    MeshData& outMesh,
                                    std::string* outError)
{
    if (!GeometryUtils::isPositiveFinite(radius)) {
        return failWith(outError, "Cone radius must be positive and finite.");
    }
    return createFrustum(radius, 0.0f, height, radialSegments, capped, false, outMesh, outError);
}

bool PrimitiveGenerator::createFrustum(float bottomRadius,
                                       float topRadius,
                                       float height,
                                       int radialSegments,
                                       bool capBottom,
                                       bool capTop,
                                       MeshData& outMesh,
                                       std::string* outError)
{
    if (!GeometryUtils::isNonNegativeFinite(bottomRadius) ||
        !GeometryUtils::isNonNegativeFinite(topRadius) ||
        !GeometryUtils::isPositiveFinite(height)) {
        return failWith(outError, "Frustum/Cone radii must be >= 0 and height must be positive finite.");
    }
    if (bottomRadius < limits::kMinDimension && topRadius < limits::kMinDimension) {
        return failWith(outError, "At least one of bottomRadius or topRadius must be positive.");
    }
    if (radialSegments < limits::kMinRadialSegments || radialSegments > limits::kMaxRadialSegments) {
        return failWith(outError, "Radial segments must be in range [3, 256].");
    }

    MeshData mesh;
    mesh.topology = graphics::PrimitiveTopology::Triangles;

    // Tính độ dốc pháp tuyến mặt bên trong mặt phẳng (r, y):
    // Vector tiếp tuyến từ đáy lên đỉnh là (topR - bottomR, height)
    // => Pháp tuyến hướng ra ngoài là (height, bottomR - topR) chuẩn hóa
    const float slopeDr = bottomRadius - topRadius;
    const float slopeLen = std::hypot(height, slopeDr);
    const float normalRadial = height / slopeLen;
    const float normalY = slopeDr / slopeLen;

    const bool hasBottomRing = (bottomRadius >= limits::kMinDimension);
    const bool hasTopRing = (topRadius >= limits::kMinDimension);

    // 1. Mặt bên (Side Surface)
    if (hasBottomRing && hasTopRing) {
        // Trường hợp Cylinder / Frustum chuẩn: sinh 2 vòng đỉnh (radialSegments + 1 cột cho UV seam)
        const auto sideBase = static_cast<std::uint32_t>(mesh.vertices.size());
        for (int i = 0; i <= radialSegments; ++i) {
            const float u = static_cast<float>(i) / static_cast<float>(radialSegments);
            const float theta = u * limits::kTwoPi;
            const float cosT = std::cos(theta);
            const float sinT = std::sin(theta);

            const glm::vec3 sideNormal = glm::normalize(glm::vec3(normalRadial * cosT, normalY, normalRadial * sinT));

            // Đỉnh vòng đáy (y = 0)
            mesh.vertices.push_back({{bottomRadius * cosT, 0.0f, bottomRadius * sinT}, sideNormal, {u, 0.0f}});
            // Đỉnh vòng đỉnh (y = height)
            mesh.vertices.push_back({{topRadius * cosT, height, topRadius * sinT}, sideNormal, {u, 1.0f}});
        }

        for (int i = 0; i < radialSegments; ++i) {
            const std::uint32_t b0 = sideBase + static_cast<std::uint32_t>(i * 2);
            const std::uint32_t t0 = b0 + 1;
            const std::uint32_t b1 = b0 + 2;
            const std::uint32_t t1 = b0 + 3;

            // Winding CCW hướng ra ngoài khi theta tăng từ +X sang +Z: (b0, t0, t1) và (b0, t1, b1)
            mesh.indices.push_back(b0);
            mesh.indices.push_back(t0);
            mesh.indices.push_back(t1);

            mesh.indices.push_back(b0);
            mesh.indices.push_back(t1);
            mesh.indices.push_back(b1);
        }
    } else if (hasBottomRing && !hasTopRing) {
        // Trường hợp Cone đỉnh nhọn tại (0, height, 0):
        // Mỗi sector tạo đúng 1 tam giác không suy biến; đỉnh apex dùng pháp tuyến tại góc giữa sector
        const auto ringBase = static_cast<std::uint32_t>(mesh.vertices.size());
        for (int i = 0; i <= radialSegments; ++i) {
            const float u = static_cast<float>(i) / static_cast<float>(radialSegments);
            const float theta = u * limits::kTwoPi;
            const float cosT = std::cos(theta);
            const float sinT = std::sin(theta);
            const glm::vec3 sideNormal = glm::normalize(glm::vec3(normalRadial * cosT, normalY, normalRadial * sinT));
            mesh.vertices.push_back({{bottomRadius * cosT, 0.0f, bottomRadius * sinT}, sideNormal, {u, 0.0f}});
        }

        const auto apexBase = static_cast<std::uint32_t>(mesh.vertices.size());
        for (int i = 0; i < radialSegments; ++i) {
            const float uMid = (static_cast<float>(i) + 0.5f) / static_cast<float>(radialSegments);
            const float thetaMid = uMid * limits::kTwoPi;
            const glm::vec3 apexNormal = glm::normalize(
                glm::vec3(normalRadial * std::cos(thetaMid), normalY, normalRadial * std::sin(thetaMid)));
            mesh.vertices.push_back({{0.0f, height, 0.0f}, apexNormal, {uMid, 1.0f}});

            const std::uint32_t b0 = ringBase + static_cast<std::uint32_t>(i);
            const std::uint32_t b1 = ringBase + static_cast<std::uint32_t>(i + 1);
            const std::uint32_t apex = apexBase + static_cast<std::uint32_t>(i);

            // (apex - b0) x (b1 - b0) hướng ra ngoài theo chiều CCW
            mesh.indices.push_back(b0);
            mesh.indices.push_back(apex);
            mesh.indices.push_back(b1);
        }
    } else {
        // Trường hợp nón ngược (bottomRadius == 0, topRadius > 0): đỉnh nhọn tại (0, 0, 0)
        const auto ringBase = static_cast<std::uint32_t>(mesh.vertices.size());
        for (int i = 0; i <= radialSegments; ++i) {
            const float u = static_cast<float>(i) / static_cast<float>(radialSegments);
            const float theta = u * limits::kTwoPi;
            const float cosT = std::cos(theta);
            const float sinT = std::sin(theta);
            const glm::vec3 sideNormal = glm::normalize(glm::vec3(normalRadial * cosT, normalY, normalRadial * sinT));
            mesh.vertices.push_back({{topRadius * cosT, height, topRadius * sinT}, sideNormal, {u, 1.0f}});
        }

        const auto apexBase = static_cast<std::uint32_t>(mesh.vertices.size());
        for (int i = 0; i < radialSegments; ++i) {
            const float uMid = (static_cast<float>(i) + 0.5f) / static_cast<float>(radialSegments);
            const float thetaMid = uMid * limits::kTwoPi;
            const glm::vec3 apexNormal = glm::normalize(
                glm::vec3(normalRadial * std::cos(thetaMid), normalY, normalRadial * std::sin(thetaMid)));
            mesh.vertices.push_back({{0.0f, 0.0f, 0.0f}, apexNormal, {uMid, 0.0f}});

            const std::uint32_t t0 = ringBase + static_cast<std::uint32_t>(i);
            const std::uint32_t t1 = ringBase + static_cast<std::uint32_t>(i + 1);
            const std::uint32_t apex = apexBase + static_cast<std::uint32_t>(i);

            mesh.indices.push_back(apex);
            mesh.indices.push_back(t0);
            mesh.indices.push_back(t1);
        }
    }

    // 2. Nắp đáy (Bottom Cap, pháp tuyến phẳng -Y, tách riêng đỉnh khỏi mặt bên)
    if (capBottom && hasBottomRing) {
        const glm::vec3 bottomNormal(0.0f, -1.0f, 0.0f);
        const auto centerIdx = static_cast<std::uint32_t>(mesh.vertices.size());
        mesh.vertices.push_back({{0.0f, 0.0f, 0.0f}, bottomNormal, {0.5f, 0.5f}});

        const auto ringStart = static_cast<std::uint32_t>(mesh.vertices.size());
        for (int i = 0; i <= radialSegments; ++i) {
            const float theta = (static_cast<float>(i) / static_cast<float>(radialSegments)) * limits::kTwoPi;
            const float cosT = std::cos(theta);
            const float sinT = std::sin(theta);
            mesh.vertices.push_back({{bottomRadius * cosT, 0.0f, bottomRadius * sinT},
                                     bottomNormal,
                                     {0.5f + 0.5f * cosT, 0.5f + 0.5f * sinT}});
        }

        for (int i = 0; i < radialSegments; ++i) {
            mesh.indices.push_back(centerIdx);
            mesh.indices.push_back(ringStart + static_cast<std::uint32_t>(i));
            mesh.indices.push_back(ringStart + static_cast<std::uint32_t>(i + 1));
        }
    }

    // 3. Nắp đỉnh (Top Cap, pháp tuyến phẳng +Y, tách riêng đỉnh khỏi mặt bên)
    if (capTop && hasTopRing) {
        const glm::vec3 topNormal(0.0f, 1.0f, 0.0f);
        const auto centerIdx = static_cast<std::uint32_t>(mesh.vertices.size());
        mesh.vertices.push_back({{0.0f, height, 0.0f}, topNormal, {0.5f, 0.5f}});

        const auto ringStart = static_cast<std::uint32_t>(mesh.vertices.size());
        for (int i = 0; i <= radialSegments; ++i) {
            const float theta = (static_cast<float>(i) / static_cast<float>(radialSegments)) * limits::kTwoPi;
            const float cosT = std::cos(theta);
            const float sinT = std::sin(theta);
            mesh.vertices.push_back({{topRadius * cosT, height, topRadius * sinT},
                                     topNormal,
                                     {0.5f + 0.5f * cosT, 0.5f - 0.5f * sinT}});
        }

        for (int i = 0; i < radialSegments; ++i) {
            mesh.indices.push_back(centerIdx);
            mesh.indices.push_back(ringStart + static_cast<std::uint32_t>(i + 1));
            mesh.indices.push_back(ringStart + static_cast<std::uint32_t>(i));
        }
    }

    mesh.recomputeBoundingBox();
    if (!mesh.validate(outError)) {
        return false;
    }
    outMesh = std::move(mesh);
    return true;
}

bool PrimitiveGenerator::createSphere(float radius,
                                      int latitudeSegments,
                                      int longitudeSegments,
                                      MeshData& outMesh,
                                      std::string* outError)
{
    if (!GeometryUtils::isPositiveFinite(radius)) {
        return failWith(outError, "Sphere radius must be positive and finite.");
    }
    if (latitudeSegments < limits::kMinLatitudeSegments || latitudeSegments > limits::kMaxLatitudeSegments ||
        longitudeSegments < limits::kMinRadialSegments || longitudeSegments > limits::kMaxRadialSegments) {
        return failWith(outError, "Sphere segments out of range (lat [3, 128], lon [3, 256]).");
    }

    MeshData mesh;
    mesh.topology = graphics::PrimitiveTopology::Triangles;

    const int cols = longitudeSegments + 1;
    const int rows = latitudeSegments + 1;
    mesh.vertices.reserve(static_cast<std::size_t>(cols * rows));

    // Tâm hình cầu đặt tại (0, radius, 0) để đáy hình cầu tiếp xúc y = 0 (pivot tâm đáy)
    const glm::vec3 center(0.0f, radius, 0.0f);

    for (int lat = 0; lat <= latitudeSegments; ++lat) {
        const float v = static_cast<float>(lat) / static_cast<float>(latitudeSegments);
        const float phi = v * limits::kPi; // 0 tại cực Bắc (+Y), pi tại cực Nam (-Y)
        const float sinPhi = std::sin(phi);
        const float cosPhi = std::cos(phi);

        for (int lon = 0; lon <= longitudeSegments; ++lon) {
            const float u = static_cast<float>(lon) / static_cast<float>(longitudeSegments);
            const float theta = u * limits::kTwoPi;
            const float cosTheta = std::cos(theta);
            const float sinTheta = std::sin(theta);

            // Pháp tuyến giải tích đơn vị hướng ra ngoài tâm cầu
            const glm::vec3 normal(sinPhi * cosTheta, cosPhi, sinPhi * sinTheta);
            const glm::vec3 pos = center + radius * normal;
            mesh.vertices.push_back({pos, normal, {u, 1.0f - v}});
        }
    }

    // Sinh chỉ mục tam giác CCW hướng ra ngoài; xử lý riêng 2 cực để KHÔNG tạo tam giác suy biến
    for (int lat = 0; lat < latitudeSegments; ++lat) {
        const int row0 = lat * cols;
        const int row1 = (lat + 1) * cols;

        for (int lon = 0; lon < longitudeSegments; ++lon) {
            const auto i00 = static_cast<std::uint32_t>(row0 + lon);
            const auto i01 = static_cast<std::uint32_t>(row0 + lon + 1);
            const auto i10 = static_cast<std::uint32_t>(row1 + lon);
            const auto i11 = static_cast<std::uint32_t>(row1 + lon + 1);

            if (lat == 0) {
                // Dải cực Bắc: chỉ phát sinh 1 tam giác không suy biến mỗi sector
                mesh.indices.push_back(i00);
                mesh.indices.push_back(i11);
                mesh.indices.push_back(i10);
            } else if (lat == latitudeSegments - 1) {
                // Dải cực Nam: chỉ phát sinh 1 tam giác không suy biến mỗi sector
                mesh.indices.push_back(i00);
                mesh.indices.push_back(i01);
                mesh.indices.push_back(i10);
            } else {
                // Các dải thân cầu: 2 tam giác CCW mỗi ô lưới
                mesh.indices.push_back(i00);
                mesh.indices.push_back(i11);
                mesh.indices.push_back(i10);

                mesh.indices.push_back(i00);
                mesh.indices.push_back(i01);
                mesh.indices.push_back(i11);
            }
        }
    }

    mesh.recomputeBoundingBox();
    if (!mesh.validate(outError)) {
        return false;
    }
    outMesh = std::move(mesh);
    return true;
}

} // namespace roomcraft::geometry
