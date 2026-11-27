#pragma once

#include "roomcraft/graphics/Vertex.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace roomcraft::graphics {

// Kiểu hình học nguyên thủy dùng cho Indexed Rendering
enum class PrimitiveTopology {
    Triangles,
    Lines
};

// Lớp Mesh quản lý dữ liệu hình học trên GPU theo mô hình RAII:
// VAO (Vertex Array Object), VBO (Vertex Buffer Object) và EBO (Element Buffer Object).
// Hỗ trợ vẽ theo chỉ mục (glDrawElements) và kiểm tra tính hợp lệ của dữ liệu trước khi upload.
class Mesh {
public:
    Mesh() = default;
    ~Mesh();

    // Không cho phép sao chép để tránh double-delete GPU handles
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Hỗ trợ di chuyển (move semantics) chuyển giao quyền sở hữu GPU buffer
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    // Kiểm tra dữ liệu hình học thuần CPU (độc lập GPU, có thể chạy unit test không cần OpenGL context)
    static bool validateData(const std::vector<Vertex>& vertices,
                             const std::vector<std::uint32_t>& indices,
                             PrimitiveTopology topology = PrimitiveTopology::Triangles,
                             std::string* outError = nullptr);

    // Khởi tạo VAO/VBO/EBO trên GPU một lần (GL_STATIC_DRAW)
    bool create(const std::vector<Vertex>& vertices,
                const std::vector<std::uint32_t>& indices,
                PrimitiveTopology topology = PrimitiveTopology::Triangles,
                std::string* outError = nullptr);

    // Vẽ mesh bằng Indexed Drawing (glDrawElements)
    void draw() const;

    // Giải phóng tường minh tài nguyên GPU trước khi OpenGL context bị hủy
    void destroy();

    bool isValid() const { return m_vao != 0 && m_indexCount > 0; }
    std::uint32_t getVertexCount() const { return m_vertexCount; }
    std::uint32_t getIndexCount() const { return m_indexCount; }
    unsigned int getVaoId() const { return m_vao; }
    PrimitiveTopology getTopology() const { return m_topology; }

    // Dữ liệu hình học mẫu (Fixture) phục vụ kiểm thử Core Graphics Engine tại Milestone M1
    // (Chưa phải PrimitiveGenerator hoàn chỉnh của M2)
    static void buildUnitCubeFixture(std::vector<Vertex>& outVertices,
                                     std::vector<std::uint32_t>& outIndices);
    static void buildGridFixture(int halfExtent,
                                 float spacing,
                                 std::vector<Vertex>& outVertices,
                                 std::vector<std::uint32_t>& outIndices);
    static void buildAxisLineFixture(const glm::vec3& start,
                                     const glm::vec3& end,
                                     std::vector<Vertex>& outVertices,
                                     std::vector<std::uint32_t>& outIndices);

private:
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    unsigned int m_ebo = 0;
    std::uint32_t m_vertexCount = 0;
    std::uint32_t m_indexCount = 0;
    PrimitiveTopology m_topology = PrimitiveTopology::Triangles;
};

} // namespace roomcraft::graphics
