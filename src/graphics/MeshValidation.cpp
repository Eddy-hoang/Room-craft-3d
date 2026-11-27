#include "roomcraft/graphics/Mesh.hpp"

#include <cmath>

namespace roomcraft::graphics {

bool Mesh::validateData(const std::vector<Vertex>& vertices,
                        const std::vector<std::uint32_t>& indices,
                        PrimitiveTopology topology,
                        std::string* outError)
{
    const auto fail = [outError](const std::string& msg) -> bool {
        if (outError != nullptr) {
            *outError = msg;
        }
        return false;
    };

    if (vertices.empty()) {
        return fail("Mesh vertex array cannot be empty.");
    }

    if (indices.empty()) {
        return fail("Mesh index array cannot be empty.");
    }

    if (topology == PrimitiveTopology::Triangles && (indices.size() % 3 != 0)) {
        return fail("Triangle mesh index count must be a multiple of 3.");
    }

    if (topology == PrimitiveTopology::Lines && (indices.size() % 2 != 0)) {
        return fail("Line mesh index count must be a multiple of 2.");
    }

    for (std::size_t i = 0; i < vertices.size(); ++i) {
        const auto& v = vertices[i];
        const bool finite = std::isfinite(v.position.x) && std::isfinite(v.position.y) && std::isfinite(v.position.z) &&
                            std::isfinite(v.normal.x) && std::isfinite(v.normal.y) && std::isfinite(v.normal.z) &&
                            std::isfinite(v.texCoord.x) && std::isfinite(v.texCoord.y);
        if (!finite) {
            return fail("Mesh contains non-finite (NaN/Inf) vertex attribute at index " + std::to_string(i) + ".");
        }
    }

    const std::size_t vertexCount = vertices.size();
    for (std::size_t i = 0; i < indices.size(); ++i) {
        if (static_cast<std::size_t>(indices[i]) >= vertexCount) {
            return fail("Mesh index out of bounds at position " + std::to_string(i) +
                        " (index=" + std::to_string(indices[i]) +
                        ", vertexCount=" + std::to_string(vertexCount) + ").");
        }
    }

    return true;
}

void Mesh::buildUnitCubeFixture(std::vector<Vertex>& outVertices,
                                std::vector<std::uint32_t>& outIndices)
{
    // Hình lập phương đơn vị 1m x 1m x 1m, tâm tại gốc tọa độ (24 đỉnh để mỗi mặt có pháp tuyến riêng)
    // Thứ tự đỉnh ngược chiều kim đồng hồ (CCW) nhìn từ bên ngoài.
    outVertices = {
        // Mặt trước (+Z)
        {{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}},

        // Mặt sau (-Z)
        {{ 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}},
        {{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}},

        // Mặt trái (-X)
        {{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
        {{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},

        // Mặt phải (+X)
        {{ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},

        // Mặt trên (+Y)
        {{-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}},

        // Mặt dưới (-Y)
        {{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}},
    };

    outIndices = {
        0,  1,  2,  2,  3,  0,  // +Z
        4,  5,  6,  6,  7,  4,  // -Z
        8,  9,  10, 10, 11, 8,  // -X
        12, 13, 14, 14, 15, 12, // +X
        16, 17, 18, 18, 19, 16, // +Y
        20, 21, 22, 22, 23, 20  // -Y
    };
}

void Mesh::buildGridFixture(int halfExtent,
                            float spacing,
                            std::vector<Vertex>& outVertices,
                            std::vector<std::uint32_t>& outIndices)
{
    outVertices.clear();
    outIndices.clear();

    if (halfExtent <= 0 || spacing <= 0.0f) {
        return;
    }

    const float maxCoord = static_cast<float>(halfExtent) * spacing;
    const glm::vec3 upNormal(0.0f, 1.0f, 0.0f);

    for (int i = -halfExtent; i <= halfExtent; ++i) {
        const float offset = static_cast<float>(i) * spacing;

        // Đường song song trục Z tại x = offset
        const auto baseIdxZ = static_cast<std::uint32_t>(outVertices.size());
        outVertices.push_back({{offset, 0.0f, -maxCoord}, upNormal, {0.0f, 0.0f}});
        outVertices.push_back({{offset, 0.0f,  maxCoord}, upNormal, {0.0f, 1.0f}});
        outIndices.push_back(baseIdxZ);
        outIndices.push_back(baseIdxZ + 1);

        // Đường song song trục X tại z = offset
        const auto baseIdxX = static_cast<std::uint32_t>(outVertices.size());
        outVertices.push_back({{-maxCoord, 0.0f, offset}, upNormal, {0.0f, 0.0f}});
        outVertices.push_back({{ maxCoord, 0.0f, offset}, upNormal, {1.0f, 0.0f}});
        outIndices.push_back(baseIdxX);
        outIndices.push_back(baseIdxX + 1);
    }
}

void Mesh::buildAxisLineFixture(const glm::vec3& start,
                                const glm::vec3& end,
                                std::vector<Vertex>& outVertices,
                                std::vector<std::uint32_t>& outIndices)
{
    outVertices = {
        {start, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {end,   {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}
    };
    outIndices = {0, 1};
}

} // namespace roomcraft::graphics
