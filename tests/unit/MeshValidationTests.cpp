#include "roomcraft/graphics/Mesh.hpp"

#include <iostream>
#include <limits>
#include <string>
#include <vector>

int runMeshValidationTests()
{
    int failures = 0;
    const auto check = [&failures](const char* name, bool condition) {
        std::cout << "  [MeshValidation] " << name << ": " << (condition ? "PASS" : "FAIL") << std::endl;
        if (!condition) {
            ++failures;
        }
    };

    // 1. Valid cube fixture & grid fixture pass validation
    {
        std::vector<roomcraft::graphics::Vertex> verts;
        std::vector<std::uint32_t> indices;
        std::string err;

        roomcraft::graphics::Mesh::buildUnitCubeFixture(verts, indices);
        const bool cubeValid = roomcraft::graphics::Mesh::validateData(
            verts, indices, roomcraft::graphics::PrimitiveTopology::Triangles, &err);
        check("Unit cube fixture data valid (24 verts, 36 indices)",
              cubeValid && verts.size() == 24 && indices.size() == 36);

        roomcraft::graphics::Mesh::buildGridFixture(4, 1.0f, verts, indices);
        const bool gridValid = roomcraft::graphics::Mesh::validateData(
            verts, indices, roomcraft::graphics::PrimitiveTopology::Lines, &err);
        check("Grid line fixture data valid", gridValid && !verts.empty() && (indices.size() % 2 == 0));
    }

    // 2. TC-M1-05: Out-of-bounds index detected
    {
        std::vector<roomcraft::graphics::Vertex> verts(3);
        std::vector<std::uint32_t> badIndices = {0, 1, 3}; // index 3 == vertexCount (out of bounds)
        std::string err;
        const bool ok = roomcraft::graphics::Mesh::validateData(
            verts, badIndices, roomcraft::graphics::PrimitiveTopology::Triangles, &err);
        check("TC-M1-05 Out-of-bounds index rejected with error message", !ok && !err.empty());
    }

    // 3. Empty vertex / index arrays rejected
    {
        std::vector<roomcraft::graphics::Vertex> emptyVerts;
        std::vector<roomcraft::graphics::Vertex> verts(3);
        std::vector<std::uint32_t> emptyIndices;
        std::vector<std::uint32_t> indices = {0, 1, 2};
        std::string err1, err2;

        const bool rejectEmptyVerts = !roomcraft::graphics::Mesh::validateData(
            emptyVerts, indices, roomcraft::graphics::PrimitiveTopology::Triangles, &err1);
        const bool rejectEmptyIndices = !roomcraft::graphics::Mesh::validateData(
            verts, emptyIndices, roomcraft::graphics::PrimitiveTopology::Triangles, &err2);
        check("Empty vertex/index buffers rejected", rejectEmptyVerts && rejectEmptyIndices);
    }

    // 4. Topology index count mismatch rejected
    {
        std::vector<roomcraft::graphics::Vertex> verts(4);
        std::vector<std::uint32_t> badTriIndices = {0, 1, 2, 3}; // 4 không chia hết cho 3
        std::vector<std::uint32_t> badLineIndices = {0, 1, 2};   // 3 không chia hết cho 2
        std::string err1, err2;

        const bool rejectTri = !roomcraft::graphics::Mesh::validateData(
            verts, badTriIndices, roomcraft::graphics::PrimitiveTopology::Triangles, &err1);
        const bool rejectLine = !roomcraft::graphics::Mesh::validateData(
            verts, badLineIndices, roomcraft::graphics::PrimitiveTopology::Lines, &err2);
        check("Mismatched triangle/line index counts rejected", rejectTri && rejectLine);
    }

    // 5. Non-finite (NaN) vertex attributes rejected
    {
        std::vector<roomcraft::graphics::Vertex> verts(3);
        verts[1].position.y = std::numeric_limits<float>::quiet_NaN();
        std::vector<std::uint32_t> indices = {0, 1, 2};
        std::string err;

        const bool rejectNan = !roomcraft::graphics::Mesh::validateData(
            verts, indices, roomcraft::graphics::PrimitiveTopology::Triangles, &err);
        check("Non-finite (NaN) vertex attribute rejected", rejectNan && !err.empty());
    }

    return failures;
}
