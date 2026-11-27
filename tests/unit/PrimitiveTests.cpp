#include "roomcraft/geometry/PrimitiveGenerator.hpp"

#include <glm/geometric.hpp>

#include <cmath>
#include <iostream>
#include <limits>
#include <string>

namespace {

constexpr float kEps = 1e-3f;

bool approxEqual(float a, float b, float eps = kEps)
{
    return std::fabs(a - b) <= eps;
}

bool checkAllNormalsUnitAndUvFinite(const roomcraft::geometry::MeshData& mesh)
{
    for (const auto& v : mesh.vertices) {
        const float nLen = glm::length(v.normal);
        if (!std::isfinite(nLen) || std::fabs(nLen - 1.0f) > 1e-2f) {
            return false;
        }
        if (!std::isfinite(v.texCoord.x) || !std::isfinite(v.texCoord.y) ||
            v.texCoord.x < -1e-4f || v.texCoord.x > 1.0001f ||
            v.texCoord.y < -1e-4f || v.texCoord.y > 1.0001f) {
            return false;
        }
    }
    return true;
}

bool checkTriangleWindingMatchesVertexNormals(const roomcraft::geometry::MeshData& mesh)
{
    for (std::size_t t = 0; t + 2 < mesh.indices.size(); t += 3) {
        const auto& v0 = mesh.vertices[mesh.indices[t]];
        const auto& v1 = mesh.vertices[mesh.indices[t + 1]];
        const auto& v2 = mesh.vertices[mesh.indices[t + 2]];
        const glm::vec3 faceCross = glm::cross(v1.position - v0.position, v2.position - v0.position);
        const float len = glm::length(faceCross);
        if (len <= 1e-9f) {
            return false;
        }
        const glm::vec3 faceNormal = faceCross / len;
        const glm::vec3 avgVertexNormal = glm::normalize(v0.normal + v1.normal + v2.normal);
        if (glm::dot(faceNormal, avgVertexNormal) <= 0.0f) {
            return false;
        }
    }
    return true;
}

} // namespace

int runPrimitiveTests()
{
    int failures = 0;
    const auto check = [&failures](const char* name, bool condition) {
        std::cout << "  [Primitives] " << name << ": " << (condition ? "PASS" : "FAIL") << std::endl;
        if (!condition) {
            ++failures;
        }
    };

    // TC-M2-02: Plane dimensions, normal +Y, CCW winding & UV
    {
        roomcraft::geometry::MeshData plane;
        std::string err;
        const bool ok = roomcraft::geometry::PrimitiveGenerator::createPlane(3.0f, 2.0f, 2, 3, plane, &err);
        const glm::vec3 size = plane.boundingBox.getSize();
        bool allNormalsUp = true;
        for (const auto& v : plane.vertices) {
            if (glm::dot(v.normal, glm::vec3(0.0f, 1.0f, 0.0f)) < 0.999f || std::fabs(v.position.y) > kEps) {
                allNormalsUp = false;
            }
        }
        check("TC-M2-02 Plane dimensions (3x2), normal +Y, CCW winding, UV",
              ok && plane.getVertexCount() == 12 && plane.getIndexCount() == 36 &&
              approxEqual(size.x, 3.0f) && approxEqual(size.y, 0.0f) && approxEqual(size.z, 2.0f) &&
              allNormalsUp && checkAllNormalsUnitAndUvFinite(plane) && checkTriangleWindingMatchesVertexNormals(plane));
    }

    // TC-M2-03: Cuboid & Cube (24 verts, 36 indices, bottom-center pivot)
    {
        roomcraft::geometry::MeshData cuboid;
        std::string err;
        const bool ok = roomcraft::geometry::PrimitiveGenerator::createCuboid(1.6f, 0.9f, 0.7f, cuboid, &err);
        const glm::vec3 size = cuboid.boundingBox.getSize();
        check("TC-M2-03 Cuboid 24 verts / 36 indices, size, bottom pivot, outward normals",
              ok && cuboid.getVertexCount() == 24 && cuboid.getIndexCount() == 36 &&
              approxEqual(size.x, 1.6f) && approxEqual(size.y, 0.9f) && approxEqual(size.z, 0.7f) &&
              approxEqual(cuboid.boundingBox.min.y, 0.0f) && approxEqual(cuboid.boundingBox.max.y, 0.9f) &&
              checkAllNormalsUnitAndUvFinite(cuboid) && checkTriangleWindingMatchesVertexNormals(cuboid));

        roomcraft::geometry::MeshData cube;
        const bool cubeOk = roomcraft::geometry::PrimitiveGenerator::createCube(1.2f, cube, &err);
        check("TC-M2-03 Cube delegates to Cuboid (1.2m)",
              cubeOk && approxEqual(cube.boundingBox.getSize().x, 1.2f) &&
              approxEqual(cube.boundingBox.getSize().y, 1.2f) &&
              approxEqual(cube.boundingBox.getSize().z, 1.2f));
    }

    // TC-M2-04: Cylinder topology, caps, UV seam & outward normals
    {
        roomcraft::geometry::MeshData cyl;
        roomcraft::geometry::MeshData cylOpen;
        std::string err;
        const int segs = 16;
        const bool okCapped = roomcraft::geometry::PrimitiveGenerator::createCylinder(0.5f, 1.8f, segs, true, cyl, &err);
        const bool okOpen = roomcraft::geometry::PrimitiveGenerator::createCylinder(0.5f, 1.8f, segs, false, cylOpen, &err);

        // Side: (segs+1)*2 verts, segs*6 indices. Each cap: 1 + (segs+1) verts, segs*3 indices.
        const std::uint32_t expectedOpenVerts = static_cast<std::uint32_t>((segs + 1) * 2);
        const std::uint32_t expectedOpenIdx = static_cast<std::uint32_t>(segs * 6);
        const std::uint32_t expectedCappedVerts = expectedOpenVerts + 2 * static_cast<std::uint32_t>(segs + 2);
        const std::uint32_t expectedCappedIdx = expectedOpenIdx + 2 * static_cast<std::uint32_t>(segs * 3);

        // Kiểm tra UV seam: đỉnh đầu và đỉnh cuối của mặt bên trùng vị trí nhưng khác u (0 vs 1)
        const bool uvSeamOk = okOpen &&
                              glm::length(cylOpen.vertices[0].position - cylOpen.vertices[static_cast<std::size_t>(segs * 2)].position) < 1e-4f &&
                              approxEqual(cylOpen.vertices[0].texCoord.x, 0.0f) &&
                              approxEqual(cylOpen.vertices[static_cast<std::size_t>(segs * 2)].texCoord.x, 1.0f);

        check("TC-M2-04 Cylinder capped/open topology, UV seam, outward normals & winding",
              okCapped && okOpen &&
              cylOpen.getVertexCount() == expectedOpenVerts && cylOpen.getIndexCount() == expectedOpenIdx &&
              cyl.getVertexCount() == expectedCappedVerts && cyl.getIndexCount() == expectedCappedIdx &&
              uvSeamOk && approxEqual(cyl.boundingBox.min.y, 0.0f) && approxEqual(cyl.boundingBox.max.y, 1.8f) &&
              checkAllNormalsUnitAndUvFinite(cyl) && checkTriangleWindingMatchesVertexNormals(cyl));
    }

    // TC-M2-05: Sphere radius, analytic normals, pole handling (no degenerate triangles)
    {
        roomcraft::geometry::MeshData sphere;
        std::string err;
        const float r = 0.75f;
        const int lat = 12;
        const int lon = 20;
        const bool ok = roomcraft::geometry::PrimitiveGenerator::createSphere(r, lat, lon, sphere, &err);

        const glm::vec3 center(0.0f, r, 0.0f);
        bool radiiAndNormalsOk = true;
        for (const auto& v : sphere.vertices) {
            const glm::vec3 delta = v.position - center;
            if (!approxEqual(glm::length(delta), r, 1e-3f)) {
                radiiAndNormalsOk = false;
            }
            if (glm::dot(v.normal, glm::normalize(delta)) < 0.999f) {
                radiiAndNormalsOk = false;
            }
        }
        // 2 dải cực mỗi dải lon tam giác + (lat-2) dải thân mỗi dải 2*lon tam giác
        const std::uint32_t expectedTris = static_cast<std::uint32_t>(2 * lon + (lat - 2) * 2 * lon);
        check("TC-M2-05 Sphere radius, analytic normals, non-degenerate poles & CCW winding",
              ok && sphere.getTriangleCount() == expectedTris && radiiAndNormalsOk &&
              checkAllNormalsUnitAndUvFinite(sphere) && checkTriangleWindingMatchesVertexNormals(sphere));
    }

    // TC-M2-06: Cone apex & Frustum sloped normals
    {
        roomcraft::geometry::MeshData cone;
        roomcraft::geometry::MeshData frustum;
        std::string err;
        const bool okCone = roomcraft::geometry::PrimitiveGenerator::createCone(0.6f, 1.2f, 20, true, cone, &err);
        const bool okFrustum = roomcraft::geometry::PrimitiveGenerator::createFrustum(0.6f, 0.25f, 1.0f, 20, true, true, frustum, &err);

        check("TC-M2-06 Cone apex (finite sloped normals, no degenerate tris) & Frustum",
              okCone && okFrustum &&
              approxEqual(cone.boundingBox.max.y, 1.2f) &&
              approxEqual(frustum.boundingBox.max.y, 1.0f) &&
              checkAllNormalsUnitAndUvFinite(cone) && checkTriangleWindingMatchesVertexNormals(cone) &&
              checkAllNormalsUnitAndUvFinite(frustum) && checkTriangleWindingMatchesVertexNormals(frustum));
    }

    // TC-M2-15: Invalid parameters rejected cleanly
    {
        roomcraft::geometry::MeshData tmp;
        std::string err;
        const bool r1 = !roomcraft::geometry::PrimitiveGenerator::createPlane(-1.0f, 2.0f, 1, 1, tmp, &err);
        const bool r2 = !roomcraft::geometry::PrimitiveGenerator::createCuboid(1.0f, 0.0f, 1.0f, tmp, &err);
        const bool r3 = !roomcraft::geometry::PrimitiveGenerator::createCylinder(0.5f, 1.0f, 2, true, tmp, &err);
        const bool r4 = !roomcraft::geometry::PrimitiveGenerator::createSphere(std::numeric_limits<float>::quiet_NaN(), 8, 8, tmp, &err);
        const bool r5 = !roomcraft::geometry::PrimitiveGenerator::createFrustum(0.0f, 0.0f, 1.0f, 12, true, true, tmp, &err);
        const bool r6 = !roomcraft::geometry::PrimitiveGenerator::createCylinder(0.5f, 1.0f, 9999, true, tmp, &err);
        check("TC-M2-15 Invalid primitive parameters rejected cleanly", r1 && r2 && r3 && r4 && r5 && r6);
    }

    return failures;
}
