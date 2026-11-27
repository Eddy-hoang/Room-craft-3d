#include "roomcraft/graphics/Mesh.hpp"

#include <glad/glad.h>

#include <cstddef>
#include <iostream>
#include <utility>

namespace roomcraft::graphics {

Mesh::~Mesh()
{
    destroy();
}

Mesh::Mesh(Mesh&& other) noexcept
    : m_vao(std::exchange(other.m_vao, 0))
    , m_vbo(std::exchange(other.m_vbo, 0))
    , m_ebo(std::exchange(other.m_ebo, 0))
    , m_vertexCount(std::exchange(other.m_vertexCount, 0))
    , m_indexCount(std::exchange(other.m_indexCount, 0))
    , m_topology(other.m_topology)
{
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
    if (this != &other) {
        destroy();
        m_vao = std::exchange(other.m_vao, 0);
        m_vbo = std::exchange(other.m_vbo, 0);
        m_ebo = std::exchange(other.m_ebo, 0);
        m_vertexCount = std::exchange(other.m_vertexCount, 0);
        m_indexCount = std::exchange(other.m_indexCount, 0);
        m_topology = other.m_topology;
    }
    return *this;
}

bool Mesh::create(const std::vector<Vertex>& vertices,
                  const std::vector<std::uint32_t>& indices,
                  PrimitiveTopology topology,
                  std::string* outError)
{
    if (!validateData(vertices, indices, topology, outError)) {
        if (outError != nullptr) {
            std::cerr << "[RoomCraft3D][ERROR] Mesh validation failed: " << *outError << std::endl;
        }
        return false;
    }

    // Dọn tài nguyên cũ nếu Mesh được tái sử dụng
    destroy();

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    // 1. Nạp mảng Vertex xen kẽ vào VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                 vertices.data(),
                 GL_STATIC_DRAW);

    // 2. Nạp mảng chỉ mục vào EBO (liên kết trực tiếp với VAO đang bind)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(std::uint32_t)),
                 indices.data(),
                 GL_STATIC_DRAW);

    // 3. Khai báo bố cục thuộc tính đỉnh bằng sizeof và offsetof
    // location = 0: position (vec3)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          static_cast<GLsizei>(sizeof(Vertex)),
                          reinterpret_cast<const void*>(offsetof(Vertex, position)));

    // location = 1: normal (vec3)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          static_cast<GLsizei>(sizeof(Vertex)),
                          reinterpret_cast<const void*>(offsetof(Vertex, normal)));

    // location = 2: texCoord (vec2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                          static_cast<GLsizei>(sizeof(Vertex)),
                          reinterpret_cast<const void*>(offsetof(Vertex, texCoord)));

    // Lưu ý: Unbind VAO TRƯỚC khi unbind EBO để giữ liên kết EBO bên trong VAO
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    m_vertexCount = static_cast<std::uint32_t>(vertices.size());
    m_indexCount = static_cast<std::uint32_t>(indices.size());
    m_topology = topology;
    return true;
}

void Mesh::draw() const
{
    if (!isValid()) {
        return;
    }

    const GLenum glMode = (m_topology == PrimitiveTopology::Lines) ? GL_LINES : GL_TRIANGLES;

    glBindVertexArray(m_vao);
    glDrawElements(glMode, static_cast<GLsizei>(m_indexCount), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void Mesh::destroy()
{
    if (m_ebo != 0) {
        glDeleteBuffers(1, &m_ebo);
        m_ebo = 0;
    }
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    m_vertexCount = 0;
    m_indexCount = 0;
}

} // namespace roomcraft::graphics
