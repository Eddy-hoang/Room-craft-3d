#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <filesystem>
#include <string>
#include <unordered_map>

namespace roomcraft::graphics {

// Lớp Shader quản lý vòng đời chương trình GLSL theo mô hình RAII:
// đọc file, biên dịch vertex/fragment shader, liên kết program, báo lỗi chi tiết và truyền uniform.
class Shader {
public:
    Shader() = default;
    ~Shader();

    // Chống sao chép để không xảy ra double-delete OpenGL program handle
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    // Cho phép di chuyển (move semantics) chuyển giao quyền sở hữu GPU handle an toàn
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    // Nạp và biên dịch shader từ đường dẫn file tương đối/tuyệt đối đã phân giải
    bool loadFromFiles(const std::filesystem::path& vertexPath,
                       const std::filesystem::path& fragmentPath,
                       std::string* outError = nullptr);

    // Biên dịch shader trực tiếp từ chuỗi mã nguồn GLSL (hữu ích cho kiểm thử và shader nội bộ)
    bool loadFromSource(const std::string& vertexSource,
                        const std::string& fragmentSource,
                        const std::string& debugLabel = "inline",
                        std::string* outError = nullptr);

    // Giải phóng tường minh chương trình shader trước khi OpenGL context bị hủy
    void destroy();

    void bind() const;
    static void unbind();

    bool isValid() const { return m_programId != 0; }
    unsigned int getProgramId() const { return m_programId; }

    // Các hàm thiết lập biến uniform cho GLSL shader
    void setInt(const std::string& name, int value);
    void setFloat(const std::string& name, float value);
    void setVec3(const std::string& name, const glm::vec3& value);
    void setVec4(const std::string& name, const glm::vec4& value);
    void setMat3(const std::string& name, const glm::mat3& value);
    void setMat4(const std::string& name, const glm::mat4& value);

private:
    static bool readTextFile(const std::filesystem::path& path,
                             std::string& outContent,
                             std::string* outError);
    static unsigned int compileStage(unsigned int shaderType,
                                     const std::string& source,
                                     const std::string& stageLabel,
                                     std::string* outError);
    int getUniformLocation(const std::string& name);

private:
    unsigned int m_programId = 0;
    std::unordered_map<std::string, int> m_uniformCache;
};

} // namespace roomcraft::graphics
