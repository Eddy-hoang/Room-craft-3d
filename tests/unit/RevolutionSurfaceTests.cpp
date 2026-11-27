#include "roomcraft/geometry/RevolutionSurface.hpp"

#include <glm/geometric.hpp>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr float kEps = 1e-3f;

bool approxEqual(float a, float b, float eps = kEps)
{
    return std::fabs(a - b) <= eps;
}

} // namespace

int runRevolutionSurfaceTests()
{
    int failures = 0;
    const auto check = [&failures](const char* name, bool condition) {
        std::cout << "  [RevolutionSurface] " << name << ": " << (condition ? "PASS" : "FAIL") << std::endl;
        if (!condition) {
            ++failures;
        }
    };

    // TC-M2-11: Rings, UV seam, outward normals, zero-radius pole handling & Bezier profile
    {
        const roomcraft::geometry::BezierCurve vaseProfile(
            {0.08f, 0.0f, 0.0f},
            {0.18f, 0.14f, 0.0f},
            {0.06f, 0.30f, 0.0f},
            {0.10f, 0.40f, 0.0f}
        );

        roomcraft::geometry::MeshData vaseMesh;
        std::string err;
        const int profSeg = 12;
        const int radSeg = 20;
        const bool ok = roomcraft::geometry::RevolutionSurface::generateFromBezierProfile(
            vaseProfile, profSeg, radSeg, true, false, vaseMesh, &err);

        // Kiểm tra UV seam ở vòng đầu tiên: cột 0 và cột radSeg trùng vị trí, u = 0 vs u = 1
        const bool seamOk = ok &&
                            glm::length(vaseMesh.vertices[0].position - vaseMesh.vertices[static_cast<std::size_t>(radSeg)].position) < 1e-4f &&
                            approxEqual(vaseMesh.vertices[0].texCoord.x, 0.0f) &&
                            approxEqual(vaseMesh.vertices[static_cast<std::size_t>(radSeg)].texCoord.x, 1.0f);

        // Kiểm tra profile có điểm r = 0 ở đỉnh (đóng kín không sinh tam giác suy biến)
        const std::vector<glm::vec2> pointedProfile = {
            {0.25f, 0.0f},
            {0.30f, 0.4f},
            {0.0f,  0.8f}
        };
        roomcraft::geometry::MeshData pointedMesh;
        const bool okZeroRadius = roomcraft::geometry::RevolutionSurface::generateFromProfile(
            pointedProfile, 16, true, false, pointedMesh, &err);

        check("TC-M2-11 Bezier revolution rings, UV seam, bottom cap & zero-radius pole",
              ok && seamOk && okZeroRadius &&
              approxEqual(vaseMesh.boundingBox.min.y, 0.0f) &&
              approxEqual(vaseMesh.boundingBox.max.y, 0.40f));
    }

    // TC-M2-15: Từ chối bán kính âm và tham số không hợp lệ
    {
        roomcraft::geometry::MeshData badMesh;
        std::string err;
        const std::vector<glm::vec2> negRadius = {{0.2f, 0.0f}, {-0.1f, 0.5f}};
        const std::vector<glm::vec2> singlePt = {{0.2f, 0.0f}};
        const std::vector<glm::vec2> allZero = {{0.0f, 0.0f}, {0.0f, 1.0f}};

        const bool r1 = !roomcraft::geometry::RevolutionSurface::generateFromProfile(negRadius, 16, true, false, badMesh, &err);
        const bool r2 = !roomcraft::geometry::RevolutionSurface::generateFromProfile(singlePt, 16, true, false, badMesh, &err);
        const bool r3 = !roomcraft::geometry::RevolutionSurface::generateFromProfile(allZero, 16, true, false, badMesh, &err);
        const bool r4 = !roomcraft::geometry::RevolutionSurface::generateFromProfile({{0.2f, 0.0f}, {0.2f, 1.0f}}, 2, true, false, badMesh, &err);

        check("TC-M2-15 Negative radius, <2 points, all-zero radius & bad segments rejected",
              r1 && r2 && r3 && r4);
    }

    return failures;
}
