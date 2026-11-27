#include "roomcraft/geometry/MeshData.hpp"

#include <glm/geometric.hpp>

#include <cmath>

namespace roomcraft::geometry {

void MeshData::clear()
{
    vertices.clear();
    indices.clear();
    boundingBox.reset();
}

void MeshData::recomputeBoundingBox()
{
    boundingBox = math::BoundingBox::fromVertices(vertices);
}

std::uint32_t MeshData::getTriangleCount() const
{
    if (topology != graphics::PrimitiveTopology::Triangles) {
        return 0;
    }
    return static_cast<std::uint32_t>(indices.size() / 3);
}

bool MeshData::validate(std::string* outError, bool checkDegenerateTriangles) const
{
    if (!graphics::Mesh::validateData(vertices, indices, topology, outError)) {
        return false;
    }

    const auto fail = [outError](const std::string& msg) -> bool {
        if (outError != nullptr) {
            *outError = msg;
        }
        return false;
    };

    if (topology == graphics::PrimitiveTopology::Triangles) {
        // Kiểm tra vector pháp tuyến chuẩn hóa (độ dài gần 1.0)
        for (std::size_t i = 0; i < vertices.size(); ++i) {
            const float nLen = glm::length(vertices[i].normal);
            if (!std::isfinite(nLen) || std::fabs(nLen - 1.0f) > 1e-2f) {
                return fail("Vertex normal at index " + std::to_string(i) +
                            " is not unit length (length=" + std::to_string(nLen) + ").");
            }
        }

        // Kiểm tra tam giác suy biến (chỉ mục trùng hoặc diện tích xấp xỉ 0)
        if (checkDegenerateTriangles) {
            for (std::size_t t = 0; t + 2 < indices.size(); t += 3) {
                const std::uint32_t i0 = indices[t];
                const std::uint32_t i1 = indices[t + 1];
                const std::uint32_t i2 = indices[t + 2];

                if (i0 == i1 || i1 == i2 || i2 == i0) {
                    return fail("Degenerate triangle with repeated vertex index at triangle " +
                                std::to_string(t / 3) + ".");
                }

                const glm::vec3& p0 = vertices[i0].position;
                const glm::vec3& p1 = vertices[i1].position;
                const glm::vec3& p2 = vertices[i2].position;
                const float twiceArea = glm::length(glm::cross(p1 - p0, p2 - p0));
                if (!std::isfinite(twiceArea) || twiceArea <= 1e-10f) {
                    return fail("Degenerate zero-area triangle at triangle " + std::to_string(t / 3) + ".");
                }
            }
        }
    }

    return true;
}

} // namespace roomcraft::geometry
