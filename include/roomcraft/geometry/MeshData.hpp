#pragma once

#include "roomcraft/graphics/Mesh.hpp"
#include "roomcraft/graphics/Vertex.hpp"
#include "roomcraft/math/BoundingBox.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace roomcraft::geometry {

// Cấu trúc dữ liệu Mesh thuần CPU — hoàn toàn độc lập với OpenGL/GPU.
// Mọi thuật toán sinh hình học trong M2 chỉ tạo ra MeshData;
// lớp graphics::Mesh chịu trách nhiệm upload dữ liệu này lên VAO/VBO/EBO trên GPU.
struct MeshData {
    std::vector<graphics::Vertex> vertices;
    std::vector<std::uint32_t> indices;
    graphics::PrimitiveTopology topology = graphics::PrimitiveTopology::Triangles;
    math::BoundingBox boundingBox;

    void clear();
    void recomputeBoundingBox();

    std::uint32_t getVertexCount() const { return static_cast<std::uint32_t>(vertices.size()); }
    std::uint32_t getIndexCount() const { return static_cast<std::uint32_t>(indices.size()); }
    std::uint32_t getTriangleCount() const;

    // Kiểm tra toàn diện dữ liệu CPU:
    // - Mảng đỉnh và chỉ mục không rỗng, chỉ mục trong biên, không chứa NaN/Inf (tái sử dụng Mesh::validateData)
    // - Với triangle mesh: kiểm tra độ dài vector pháp tuyến ≈ 1 và phát hiện tam giác suy biến (diện tích = 0)
    bool validate(std::string* outError = nullptr, bool checkDegenerateTriangles = true) const;
};

} // namespace roomcraft::geometry
