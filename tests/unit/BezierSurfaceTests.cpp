#include "roomcraft/geometry/BezierSurface.hpp"

#include <glm/geometric.hpp>

#include <cmath>
#include <iostream>
#include <string>

namespace {

constexpr float kEps = 1e-3f;

bool approxVec3(const glm::vec3& a, const glm::vec3& b, float eps = kEps)
{
    return glm::length(a - b) <= eps;
}

} // namespace

int runBezierSurfaceTests()
{
    int failures = 0;
    const auto check = [&failures](const char* name, bool condition) {
        std::cout << "  [BezierSurface] " << name << ": " << (condition ? "PASS" : "FAIL") << std::endl;
        if (!condition) {
            ++failures;
        }
    };

    // TC-M2-09: 4 điểm góc của mặt cong Bézier & mặt phẳng điều khiển phẳng
    {
        const auto curved = roomcraft::geometry::BezierSurface::createCurvedWavePatch(2.0f, 2.0f, 0.6f);
        const auto& grid = curved.getControlPoints();
        glm::vec3 c00, c10, c01, c11;
        const bool ok = curved.evaluate(0.0f, 0.0f, c00) &&
                        curved.evaluate(1.0f, 0.0f, c10) &&
                        curved.evaluate(0.0f, 1.0f, c01) &&
                        curved.evaluate(1.0f, 1.0f, c11);
        check("TC-M2-09 BezierSurface matches all 4 corner control points",
              ok &&
              approxVec3(c00, grid[0][0]) &&
              approxVec3(c10, grid[3][0]) &&
              approxVec3(c01, grid[0][3]) &&
              approxVec3(c11, grid[3][3]));
    }

    // TC-M2-10: Flat patch normals (+Y), curved patch normals, topology counts & invalid input rejection
    {
        const auto flat = roomcraft::geometry::BezierSurface::createFlatPatch(2.0f, 2.0f);
        roomcraft::geometry::MeshData flatMesh;
        std::string err;
        const bool okFlat = flat.generateMesh(6, 5, flatMesh, &err);
        bool flatNormalsUp = okFlat;
        for (const auto& v : flatMesh.vertices) {
            if (glm::dot(v.normal, glm::vec3(0.0f, 1.0f, 0.0f)) < 0.999f || std::fabs(v.position.y) > kEps) {
                flatNormalsUp = false;
            }
        }

        const auto curved = roomcraft::geometry::BezierSurface::createCurvedWavePatch(2.0f, 2.0f, 0.5f);
        roomcraft::geometry::MeshData curvedMesh;
        const bool okCurved = curved.generateMesh(8, 6, curvedMesh, &err);
        const bool countsOk = okCurved &&
                              curvedMesh.getVertexCount() == 9 * 7 &&
                              curvedMesh.getIndexCount() == 8 * 6 * 6;

        // Kiểm tra winding khớp hướng pháp tuyến
        bool windingOk = countsOk;
        for (std::size_t t = 0; t + 2 < curvedMesh.indices.size(); t += 3) {
            const auto& v0 = curvedMesh.vertices[curvedMesh.indices[t]];
            const auto& v1 = curvedMesh.vertices[curvedMesh.indices[t + 1]];
            const auto& v2 = curvedMesh.vertices[curvedMesh.indices[t + 2]];
            const glm::vec3 fn = glm::normalize(glm::cross(v1.position - v0.position, v2.position - v0.position));
            const glm::vec3 avgN = glm::normalize(v0.normal + v1.normal + v2.normal);
            if (glm::dot(fn, avgN) <= 0.0f) {
                windingOk = false;
            }
        }

        // Từ chối số segment <= 0 và mặt phẳng bị sụp hoàn toàn về 1 điểm
        roomcraft::geometry::MeshData badMesh;
        const bool rejectBadSeg = !curved.generateMesh(0, 4, badMesh, &err) &&
                                  !curved.generateMesh(4, 200, badMesh, &err);

        roomcraft::geometry::BezierSurface::ControlGrid collapsed{};
        for (auto& row : collapsed) {
            row.fill(glm::vec3(0.0f));
        }
        const roomcraft::geometry::BezierSurface degenerateSurface(collapsed);
        const bool rejectDegenerate = !degenerateSurface.generateMesh(4, 4, badMesh, &err);

        check("TC-M2-10 Flat/Curved normals, topology counts, winding & invalid patch rejection",
              flatNormalsUp && windingOk && rejectBadSeg && rejectDegenerate);
    }

    return failures;
}
